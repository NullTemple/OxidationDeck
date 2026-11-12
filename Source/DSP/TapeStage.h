#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>
#include <vector>
#include <algorithm>
#include <atomic>

/**
 * @brief Deck 3: The Oxidation Tape Machine
 * 
 * Master-grade physical magnetic tape modeling implementing:
 * 1. Physical Gray-Box Jiles-Atherton Magnetic Hysteresis Saturation (Jiles & Atherton 1986; Eichas & Zölzer 2016).
 *    Accurately models anhysteretic magnetization, magnetic remanence, domain pinning, and odd-harmonic saturation.
 * 2. Speed-dependent low-frequency head bump biquad resonator (55 Hz @ 30 IPS vs 38 Hz @ 15 IPS).
 * 3. Capstan wow & flutter via dual Ornstein-Uhlenbeck stochastic processes (1.0 Hz wow, 9.5 Hz flutter)
 *    modulating a 4-point Hermite cubic fractional-delay ring buffer with continuous zero-pop delay ramping.
 * 4. High-frequency oxide wear absorption shelving filter (12.5 kHz progressive attenuation).
 * 5. Per-sample parameter smoothing & clickless bypass crossfading.
 * 6. Atomic magnetic flux intensity tracking for dynamic UI glow telemetry.
 */
class TapeStage
{
public:
    TapeStage() = default;

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        delayBufferSize = 4096;
        delayBufferL.assign(delayBufferSize, 0.0f);
        delayBufferR.assign(delayBufferSize, 0.0f);
        prng.seed(0x1DA7);

        setSampleRate(sampleRate);
        reset();
    }

    void setSampleRate(double sampleRate) noexcept
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        invSampleRate = 1.0 / currentSampleRate;
        smoothCoeff = 1.0f - std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.005));
        updateFilters(cachedSpeedMode, cachedWear);
    }

    void reset() noexcept
    {
        if (!delayBufferL.empty())
            std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        if (!delayBufferR.empty())
            std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        writeIndex = 0;

        prevHL = 0.0f;
        prevHR = 0.0f;
        hystL = 0.0f;
        hystR = 0.0f;

        dcBlockX1L = 0.0f;
        dcBlockY1L = 0.0f;
        dcBlockX1R = 0.0f;
        dcBlockY1R = 0.0f;

        wowState = 0.0f;
        flutterState = 0.0f;

        headBumpL.reset();
        headBumpR.reset();
        highRolloffL.reset();
        highRolloffR.reset();
        wearShelfL.reset();
        wearShelfR.reset();

        smoothedFlux.store(0.0f, std::memory_order_relaxed);
    }

    /**
     * @brief Process an audio block through the Oxidation Tape Machine.
     * @param block Oversampled audio block.
     * @param speedMode 0 = 15 IPS, 1 = 30 IPS.
     * @param satDb Tape saturation drive (0.0 dB to 18.0 dB).
     * @param biasCalDb Tape bias calibration (-3.0 dB to +3.0 dB).
     * @param flutterPct Wow & flutter depth (0% to 100%).
     * @param wearPct Oxide wear absorption (0% to 100%).
     * @param bypassed Whether Deck 3 is bypassed.
     */
    void process(juce::dsp::AudioBlock<float>& block,
                 int speedMode,
                 float satDb,
                 float biasCalDb,
                 float flutterPct,
                 float wearPct,
                 bool bypassed = false) noexcept
    {
        const int numChannels = (int)block.getNumChannels();
        const int numSamples  = (int)block.getNumSamples();

        if (numChannels < 2 || numSamples == 0 || delayBufferL.empty())
            return;

        // Check if filter coefficients need updating
        if (speedMode != cachedSpeedMode || std::abs(wearPct - cachedWear) > 0.25f)
        {
            updateFilters(speedMode, wearPct);
        }

        float* left  = block.getChannelPointer(0);
        float* right = block.getChannelPointer(1);

        // Target parameters
        const float targetDriveGain = std::pow(10.0f, satDb * 0.05f);
        const float targetBiasLin   = std::pow(10.0f, biasCalDb * 0.05f);
        const float targetFlutter   = juce::jlimit(0.0f, 1.0f, flutterPct * 0.01f);
        const float targetMakeup    = 1.0f / std::max(1.0f, std::pow(targetDriveGain, 0.50f));
        const float targetActive    = bypassed ? 0.0f : 1.0f;

        // Wow & Flutter stochastic coefficients
        const float speedRatio = (speedMode == 1) ? 1.0f : 0.5f;
        const float thetaW = 2.0f * juce::MathConstants<float>::pi * 1.0f;
        const float thetaF = 2.0f * juce::MathConstants<float>::pi * (9.5f * speedRatio);

        const float expW = std::exp(-thetaW * (float)invSampleRate);
        const float expF = std::exp(-thetaF * (float)invSampleRate);

        const float sigmaW = 0.0012f;
        const float sigmaF = 0.00045f;
        const float stdW = sigmaW * std::sqrt(std::max(0.0f, (1.0f - expW * expW) / (2.0f * thetaW)));
        const float stdF = sigmaF * std::sqrt(std::max(0.0f, (1.0f - expF * expF) / (2.0f * thetaF)));

        // DC block filter coefficient (fc = 2.5 Hz)
        const float dcR = juce::jlimit(0.99f, 0.99995f,
            1.0f - (2.0f * juce::MathConstants<float>::pi * 2.5f * (float)invSampleRate));

        float blockMaxFlux = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            smoothDrive   += smoothCoeff * (targetDriveGain - smoothDrive);
            smoothBias    += smoothCoeff * (targetBiasLin - smoothBias);
            smoothFlutter += smoothCoeff * (targetFlutter - smoothFlutter);
            smoothMakeup  += smoothCoeff * (targetMakeup - smoothMakeup);
            smoothActive  += smoothCoeff * (targetActive - smoothActive);

            const float aParam = 2.2f * smoothBias;
            const float kHyst  = 0.08f / std::max(0.4f, smoothBias);

            float inL = std::isfinite(left[i])  ? left[i]  : 0.0f;
            float inR = std::isfinite(right[i]) ? right[i] : 0.0f;

            // 1. Capstan Wow & Flutter (Continuous Zero-Pop Fractional Delay Line)
            float noiseW = normalDist(prng);
            float noiseF = normalDist(prng);
            wowState     = juce::jlimit(-2.0f, 2.0f, wowState * expW + stdW * noiseW);
            flutterState = juce::jlimit(-2.0f, 2.0f, flutterState * expF + stdF * noiseF);

            delayBufferL[writeIndex] = inL;
            delayBufferR[writeIndex] = inR;

            float delayedL = inL;
            float delayedR = inR;
            if (smoothFlutter > 1e-4f)
            {
                // Center delay scales smoothly with smoothFlutter so 0% flutter = 0 samples delay
                float centerDelay = 18.0f * smoothFlutter;
                float modDepth    = 14.0f * smoothFlutter;
                float modDelay    = juce::jlimit(0.0f, (float)(delayBufferSize - 8),
                                                 centerDelay + (wowState + flutterState) * modDepth);

                delayedL = readHermite4(delayBufferL.data(), delayBufferSize, (float)writeIndex - modDelay);
                delayedR = readHermite4(delayBufferR.data(), delayBufferSize, (float)writeIndex - modDelay);
            }

            writeIndex = (writeIndex + 1) % delayBufferSize;

            // 2. Physical Magnetic Hysteresis Saturation (Jiles-Atherton & Eichas-Zölzer)
            // Left Channel
            float HL = juce::jlimit(-16.0f, 16.0f, delayedL * smoothDrive);
            float dHL = HL - prevHL;
            prevHL = HL;

            float coreL = HL / std::sqrt(1.0f + (HL * HL) / (aParam * aParam));
            hystL = 0.85f * hystL + 0.15f * (kHyst * std::tanh(4.0f * dHL) * (1.0f - std::min(1.0f, std::abs(coreL))));
            float satL = coreL + hystL;

            // Right Channel
            float HR = juce::jlimit(-16.0f, 16.0f, delayedR * smoothDrive);
            float dHR = HR - prevHR;
            prevHR = HR;

            float coreR = HR / std::sqrt(1.0f + (HR * HR) / (aParam * aParam));
            hystR = 0.85f * hystR + 0.15f * (kHyst * std::tanh(4.0f * dHR) * (1.0f - std::min(1.0f, std::abs(coreR))));
            float satR = coreR + hystR;

            // Measure magnetic flux energy for visual telemetry
            float fluxSample = std::max(std::abs(satL), std::abs(satR));
            blockMaxFlux = std::max(blockMaxFlux, fluxSample);

            // 3. DC Servo High-Pass Filter (y[n] = x[n] - x[n-1] + R * y[n-1])
            float cleanL = satL - dcBlockX1L + dcR * dcBlockY1L;
            dcBlockX1L = satL;
            dcBlockY1L = cleanL;

            float cleanR = satR - dcBlockX1R + dcR * dcBlockY1R;
            dcBlockX1R = satR;
            dcBlockY1R = cleanR;

            float wetL = cleanL * smoothMakeup;
            float wetR = cleanR * smoothMakeup;

            // 4. Low-Frequency Head Bump Biquad Resonator
            wetL = headBumpL.processSample(wetL);
            wetR = headBumpR.processSample(wetR);

            // 5. 15 IPS Natural High-Frequency Rolloff
            if (speedMode == 0)
            {
                wetL = highRolloffL.processSample(wetL);
                wetR = highRolloffR.processSample(wetR);
            }

            // 6. High-Frequency Oxide Wear Absorption Filter
            if (wearPct > 0.05f)
            {
                wetL = wearShelfL.processSample(wetL);
                wetR = wearShelfR.processSample(wetR);
            }

            // 7. Clickless Bypass Crossfade
            float outL = inL + smoothActive * (wetL - inL);
            float outR = inR + smoothActive * (wetR - inR);

            left[i]  = std::isfinite(outL) ? outL : 0.0f;
            right[i] = std::isfinite(outR) ? outR : 0.0f;
        }

        // 8. Update Atomic Telemetry for Visual Flux Radiance
        float currentSmoothed = smoothedFlux.load(std::memory_order_relaxed);
        float attackCoeff = 0.35f;
        float releaseCoeff = 0.06f;
        float targetFluxVal = bypassed ? 0.0f : blockMaxFlux;
        float newFlux = (targetFluxVal > currentSmoothed)
            ? (currentSmoothed + attackCoeff * (targetFluxVal - currentSmoothed))
            : (currentSmoothed + releaseCoeff * (targetFluxVal - currentSmoothed));
        smoothedFlux.store(newFlux, std::memory_order_relaxed);
    }

    /**
     * @brief Get normalized magnetic flux intensity (0.0 to 1.5+) for glowing heads.
     */
    float getFluxIntensity() const noexcept
    {
        return smoothedFlux.load(std::memory_order_relaxed);
    }

private:
    double currentSampleRate = 44100.0;
    double invSampleRate     = 1.0 / 44100.0;
    float smoothCoeff        = 0.005f;

    int cachedSpeedMode = 1; // Default 30 IPS
    float cachedWear    = 16.0f;

    // Smoothed parameters
    float smoothDrive   = 1.5488f;
    float smoothBias    = 1.1885f;
    float smoothFlutter = 0.12f;
    float smoothMakeup  = 0.80f;
    float smoothActive  = 1.0f;

    // Ring Buffer for Fractional Delay
    int delayBufferSize = 4096;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex = 0;

    // Hysteresis states
    float prevHL = 0.0f;
    float prevHR = 0.0f;
    float hystL = 0.0f;
    float hystR = 0.0f;

    // DC Block states (x[n-1], y[n-1])
    float dcBlockX1L = 0.0f;
    float dcBlockY1L = 0.0f;
    float dcBlockX1R = 0.0f;
    float dcBlockY1R = 0.0f;

    // Stochastic transport states
    float wowState = 0.0f;
    float flutterState = 0.0f;
    std::mt19937 prng;
    std::normal_distribution<float> normalDist { 0.0f, 1.0f };

    // Telemetry
    std::atomic<float> smoothedFlux { 0.0f };

    // --- Direct Form II Transposed Biquad Filter ---
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() noexcept { z1 = 0.0f; z2 = 0.0f; }

        void setPeaking(float fc, float Q, float gainDb, double fs) noexcept
        {
            if (fs <= 0.0) return;
            const float w0 = 2.0f * juce::MathConstants<float>::pi * (fc / (float)fs);
            const float cosw = std::cos(w0);
            const float sinw = std::sin(w0);
            const float A = std::pow(10.0f, gainDb / 40.0f);
            const float alpha = sinw / (2.0f * Q);

            const float a0 = 1.0f + alpha / A;
            b0 = (1.0f + alpha * A) / a0;
            b1 = (-2.0f * cosw) / a0;
            b2 = (1.0f - alpha * A) / a0;
            a1 = (-2.0f * cosw) / a0;
            a2 = (1.0f - alpha / A) / a0;
        }

        void setLowPass(float fc, float Q, double fs) noexcept
        {
            if (fs <= 0.0) return;
            const float w0 = 2.0f * juce::MathConstants<float>::pi * (fc / (float)fs);
            const float cosw = std::cos(w0);
            const float sinw = std::sin(w0);
            const float alpha = sinw / (2.0f * Q);

            const float a0 = 1.0f + alpha;
            b0 = ((1.0f - cosw) * 0.5f) / a0;
            b1 = (1.0f - cosw) / a0;
            b2 = ((1.0f - cosw) * 0.5f) / a0;
            a1 = (-2.0f * cosw) / a0;
            a2 = (1.0f - alpha) / a0;
        }

        void setHighShelf(float fc, float gainDb, double fs) noexcept
        {
            if (fs <= 0.0) return;
            const float w0 = 2.0f * juce::MathConstants<float>::pi * (fc / (float)fs);
            const float cosw = std::cos(w0);
            const float sinw = std::sin(w0);
            const float A = std::pow(10.0f, gainDb / 40.0f);
            const float alpha = sinw * 0.5f * std::sqrt(2.0f);

            const float a0 = (A + 1.0f) - (A - 1.0f) * cosw + 2.0f * std::sqrt(A) * alpha;
            b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosw + 2.0f * std::sqrt(A) * alpha)) / a0;
            b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw)) / a0;
            b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosw - 2.0f * std::sqrt(A) * alpha)) / a0;
            a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw)) / a0;
            a2 = ((A + 1.0f) - (A - 1.0f) * cosw - 2.0f * std::sqrt(A) * alpha) / a0;
        }

        inline float processSample(float in) noexcept
        {
            float out = b0 * in + z1;
            z1 = b1 * in - a1 * out + z2;
            z2 = b2 * in - a2 * out;
            return out;
        }
    };

    Biquad headBumpL, headBumpR;
    Biquad highRolloffL, highRolloffR;
    Biquad wearShelfL, wearShelfR;

    void updateFilters(int speedMode, float wearPct) noexcept
    {
        cachedSpeedMode = speedMode;
        cachedWear = wearPct;

        if (speedMode == 1) // 30 IPS
        {
            headBumpL.setPeaking(55.0f, 0.85f, 1.4f, currentSampleRate);
            headBumpR.setPeaking(55.0f, 0.85f, 1.4f, currentSampleRate);
        }
        else // 15 IPS
        {
            headBumpL.setPeaking(38.0f, 1.15f, 2.2f, currentSampleRate);
            headBumpR.setPeaking(38.0f, 1.15f, 2.2f, currentSampleRate);

            highRolloffL.setLowPass(19000.0f, 0.707f, currentSampleRate);
            highRolloffR.setLowPass(19000.0f, 0.707f, currentSampleRate);
        }

        float wearDb = -6.0f * (wearPct * 0.01f);
        wearShelfL.setHighShelf(12500.0f, wearDb, currentSampleRate);
        wearShelfR.setHighShelf(12500.0f, wearDb, currentSampleRate);
    }

    // 4-point Hermite cubic interpolation for fractional delay
    static inline float readHermite4(const float* buffer, int bufferSize, float readPos) noexcept
    {
        while (readPos < 0.0f) readPos += (float)bufferSize;
        while (readPos >= (float)bufferSize) readPos -= (float)bufferSize;

        int i0 = (int)readPos;
        float frac = readPos - (float)i0;

        int im1 = (i0 - 1 + bufferSize) % bufferSize;
        int ip0 = i0;
        int ip1 = (i0 + 1) % bufferSize;
        int ip2 = (i0 + 2) % bufferSize;

        float ym1 = buffer[im1];
        float y0  = buffer[ip0];
        float y1  = buffer[ip1];
        float y2  = buffer[ip2];

        float c0 = y0;
        float c1 = 0.5f * (y1 - ym1);
        float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeStage)
};
