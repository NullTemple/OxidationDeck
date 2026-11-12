#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

/**
 * @brief Stage 2: Master Bus Dynamics (The Glue Engine)
 *
 * Peer-Reviewed Reference:
 *   Giannoulis, Massberg & Reiss (2012), "Digital Dynamic Range Compressor Design—A Tutorial and Analysis",
 *   Journal of the Audio Engineering Society (JAES), 60(6), 399-408.
 *
 * Features:
 *   - Dual Topology: Feed-Forward VCA (SSL G-Bus / API 2500) vs Feedback Vari-Mu (Fairchild 670 / Manley)
 *   - Exact 2nd-order Butterworth Sidechain HPF (30 Hz - 250 Hz) via canonical bilinear transform
 *   - Smooth 2nd-Order Quadratic Knee (W = 6 dB) eliminating slope distortion
 *   - Decoupled Envelope Detection with Dual-Time-Constant Auto-Release (50ms fast / 1200ms slow)
 *   - 80% Stereo-Linked + 20% Independent Channel Detection for organic L/R & M/S GR telemetry
 *   - Clickless Bypass & Sidechain Listen Crossfaders
 */
class DynamicsStage
{
public:
    DynamicsStage() = default;

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        setSampleRate(sampleRate);
        reset();
    }

    void setSampleRate(double sampleRate) noexcept
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        smoothCoeff = 1.0f - std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.005));
        float targetCut = (currentScHpfCutoff > 0.0f) ? currentScHpfCutoff : 85.0f;
        currentScHpfCutoff = -1.0f;
        updateSidechainFilter(targetCut);
    }

    void reset() noexcept
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            scHpfState1[ch] = 0.0f;
            scHpfState2[ch] = 0.0f;
            envDetector[ch] = 0.0f;
            smoothedGRDb[ch] = 0.0f;
            prevOutput[ch] = 0.0f;
            transientDensity[ch] = 0.0f;
            lastGRDb[ch] = 0.0f;
        }
    }

    void process(juce::dsp::AudioBlock<float>& block,
                 int modeIndex,            // 0: VCA, 1: Vari-Mu
                 float thresholdDb,        // [-30.0, +10.0] dB
                 int ratioIndex,           // 0: 1.5:1, 1: 2:1, 2: 4:1, 3: 10:1
                 int attackIndex,          // 0: 0.1ms, 1: 0.3ms, 2: 1ms, 3: 3ms, 4: 10ms, 5: 30ms
                 int releaseIndex,         // 0: 100ms, 1: 300ms, 2: 600ms, 3: 1.2s, 4: Auto
                 float scHpfCutoffHz,      // [30.0, 250.0] Hz
                 bool scListen,
                 bool bypassed = false) noexcept
    {
        const int numChannels = static_cast<int>(block.getNumChannels());
        const int numSamples  = static_cast<int>(block.getNumSamples());

        if (numChannels < 2 || numSamples == 0)
            return;

        updateSidechainFilter(scHpfCutoffHz);

        // 1. Resolve Ratio
        const float ratios[] = { 1.5f, 2.0f, 4.0f, 10.0f };
        float R = ratios[juce::jlimit(0, 3, ratioIndex)];

        // 2. Resolve Attack Ballistics Alpha
        const float attackTimesSec[] = { 0.0001f, 0.0003f, 0.001f, 0.003f, 0.010f, 0.030f };
        float attSec = attackTimesSec[juce::jlimit(0, 5, attackIndex)];
        float alphaAtt = std::exp(-1.0f / static_cast<float>(currentSampleRate * attSec));

        // 3. Resolve Release Ballistics Alpha (Fixed vs Dual-Time-Constant Auto)
        const float releaseTimesSec[] = { 0.100f, 0.300f, 0.600f, 1.200f };
        bool isAutoRelease = (releaseIndex >= 4);
        float alphaRelFixed = isAutoRelease ? 0.0f
            : std::exp(-1.0f / static_cast<float>(currentSampleRate * releaseTimesSec[juce::jlimit(0, 3, releaseIndex)]));

        // Auto-release dual time constants (Fast: 50ms, Slow: 1200ms)
        float alphaFast = std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.050));
        float alphaSlow = std::exp(-1.0f / static_cast<float>(currentSampleRate * 1.200));
        float densityCoeff = std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.200));

        // Quadratic 6 dB soft knee parameters (Giannoulis et al. Eq. 4)
        const float kneeWidth = 6.0f;
        const float halfKnee  = 3.0f;

        // Tamed analog makeup gain (subtle transparent glue lift, capped at +1.2 dB)
        float makeupDb = std::min(1.2f, std::max(0.0f, (-thresholdDb) * 0.05f));
        float targetMakeup = std::pow(10.0f, makeupDb * 0.05f);
        float targetActive = bypassed ? 0.0f : 1.0f;
        float targetListen = (scListen && !bypassed) ? 1.0f : 0.0f;

        float maxGrL = 0.0f;
        float maxGrR = 0.0f;

        float* left  = block.getChannelPointer(0);
        float* right = block.getChannelPointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            smoothThresh += smoothCoeff * (thresholdDb - smoothThresh);
            smoothMakeup += smoothCoeff * (targetMakeup - smoothMakeup);
            smoothActive += smoothCoeff * (targetActive - smoothActive);
            smoothListen += smoothCoeff * (targetListen - smoothListen);

            float inL = std::isfinite(left[i])  ? left[i]  : 0.0f;
            float inR = std::isfinite(right[i]) ? right[i] : 0.0f;

            // A. Detector Source: Feed-Forward (VCA) vs Feedback (Vari-Mu)
            // When in SC-Listen mode, always filter the input directly so Vari-Mu audition never mutes
            float detInL = (modeIndex == 0 || scListen) ? inL : (0.35f * inL + 0.65f * prevOutput[0]);
            float detInR = (modeIndex == 0 || scListen) ? inR : (0.35f * inR + 0.65f * prevOutput[1]);

            // B. Sidechain 2nd-order Butterworth HPF
            float scL = processSidechainHPF(detInL, 0);
            float scR = processSidechainHPF(detInR, 1);

            // 80% Stereo Linked + 20% Independent Channel Detection for organic L/R & M/S metering
            float rectifiedL = std::abs(scL);
            float rectifiedR = std::abs(scR);
            float maxLinked  = std::max(rectifiedL, rectifiedR);

            float detLevelCh[2] = {
                0.80f * maxLinked + 0.20f * rectifiedL,
                0.80f * maxLinked + 0.20f * rectifiedR
            };

            float outWet[2] = { inL, inR };

            for (int ch = 0; ch < 2; ++ch)
            {
                float channelInput = (ch == 0) ? inL : inR;
                float detLevel = detLevelCh[ch];

                // C. Decoupled Peak Envelope Tracking (Giannoulis et al. Section 3.3)
                if (detLevel > envDetector[ch])
                    envDetector[ch] = alphaAtt * envDetector[ch] + (1.0f - alphaAtt) * detLevel;
                else
                {
                    float alphaRel = alphaRelFixed;
                    if (isAutoRelease)
                    {
                        float delta = std::abs(detLevel - envDetector[ch]);
                        transientDensity[ch] = densityCoeff * transientDensity[ch] + (1.0f - densityCoeff) * delta;
                        float kappa = juce::jlimit(0.05f, 0.95f, transientDensity[ch] * 3.0f);
                        alphaRel = kappa * alphaFast + (1.0f - kappa) * alphaSlow;
                    }
                    envDetector[ch] = alphaRel * envDetector[ch] + (1.0f - alphaRel) * detLevel;
                }

                // Convert smoothed peak envelope to decibels
                float envDbVal = (envDetector[ch] > 1e-6f)
                    ? (20.0f * std::log10(envDetector[ch]))
                    : -120.0f;

                // D. Quadratic Knee Static Characteristic (Giannoulis et al. Eq. 4-6)
                float currentRatio = R;
                if (modeIndex == 1) // Vari-Mu: Progressive variable-mu ratio
                {
                    float overshoot = std::max(0.0f, envDbVal - smoothThresh);
                    currentRatio = std::min(16.0f, R + overshoot * 0.22f);
                }

                float delta = envDbVal - smoothThresh;
                float targetGRDb = 0.0f;

                if (delta <= -halfKnee)
                {
                    targetGRDb = 0.0f;
                }
                else if (std::abs(delta) < halfKnee)
                {
                    float xKnee = delta + halfKnee;
                    targetGRDb = ((1.0f / currentRatio) - 1.0f) * (xKnee * xKnee) / (2.0f * kneeWidth);
                }
                else
                {
                    targetGRDb = ((1.0f / currentRatio) - 1.0f) * delta;
                }

                targetGRDb = std::min(0.0f, targetGRDb);
                smoothedGRDb[ch] = 0.90f * smoothedGRDb[ch] + 0.10f * targetGRDb;

                // E. Gain Application
                float gainLinear = std::pow(10.0f, smoothedGRDb[ch] * 0.05f) * smoothMakeup;
                float yOut = channelInput * gainLinear;

                // Vari-Mu Output Stage Warmth (gentle tube cathode saturation)
                if (modeIndex == 1)
                {
                    yOut = std::tanh(yOut * 1.08f) * 0.926f;
                }

                prevOutput[ch] = yOut;
                outWet[ch] = yOut;
            }

            maxGrL = std::min(maxGrL, smoothedGRDb[0]);
            maxGrR = std::min(maxGrR, smoothedGRDb[1]);

            // Crossfade between normal wet output and SC Listen
            float compL = outWet[0] + smoothListen * (scL - outWet[0]);
            float compR = outWet[1] + smoothListen * (scR - outWet[1]);

            // Clickless Bypass Crossfade
            left[i]  = inL + smoothActive * (compL - inL);
            right[i] = inR + smoothActive * (compR - inR);
        }

        if (bypassed || scListen)
        {
            lastGRDb[0] = 0.0f;
            lastGRDb[1] = 0.0f;
        }
        else
        {
            lastGRDb[0] = maxGrL;
            lastGRDb[1] = maxGrR;
        }
    }

    float getGainReductionL() const noexcept { return lastGRDb[0]; }
    float getGainReductionR() const noexcept { return lastGRDb[1]; }

private:
    double currentSampleRate = 44100.0;
    float currentScHpfCutoff = -1.0f;
    float smoothCoeff = 0.005f;

    float smoothThresh = -14.0f;
    float smoothMakeup = 1.08f;
    float smoothActive = 1.0f;
    float smoothListen = 0.0f;

    // 2nd-order Butterworth HPF biquad coefficients
    float hpfB0 = 1.0f, hpfB1 = -2.0f, hpfB2 = 1.0f;
    float hpfA1 = 0.0f, hpfA2 = 0.0f;

    // Filter Direct Form II Transposed states per channel
    float scHpfState1[2] { 0.0f, 0.0f };
    float scHpfState2[2] { 0.0f, 0.0f };

    // Dynamic states per channel
    float envDetector[2] { 0.0f, 0.0f };
    float smoothedGRDb[2] { 0.0f, 0.0f };
    float prevOutput[2] { 0.0f, 0.0f };
    float transientDensity[2] { 0.0f, 0.0f };
    float lastGRDb[2] { 0.0f, 0.0f };

    void updateSidechainFilter(float cutoffHz) noexcept
    {
        cutoffHz = juce::jlimit(20.0f, 400.0f, cutoffHz);
        if (std::abs(cutoffHz - currentScHpfCutoff) < 0.2f)
            return;

        currentScHpfCutoff = cutoffHz;

        // Canonical Bilinear Transform 2nd-order Butterworth High-Pass (Q = 1 / sqrt(2) ≈ 0.7071)
        double K = std::tan(juce::MathConstants<double>::pi * cutoffHz / currentSampleRate);
        double K2 = K * K;
        double sqrt2K = 1.414213562373095 * K;
        double norm = 1.0 / (1.0 + sqrt2K + K2);

        hpfB0 = static_cast<float>(norm);
        hpfB1 = static_cast<float>(-2.0 * norm);
        hpfB2 = static_cast<float>(norm);
        hpfA1 = static_cast<float>(2.0 * (K2 - 1.0) * norm);
        hpfA2 = static_cast<float>((1.0 - sqrt2K + K2) * norm);
    }

    inline float processSidechainHPF(float in, int ch) noexcept
    {
        float out = hpfB0 * in + scHpfState1[ch];
        scHpfState1[ch] = hpfB1 * in - hpfA1 * out + scHpfState2[ch];
        scHpfState2[ch] = hpfB2 * in - hpfA2 * out;
        return out;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicsStage)
};
