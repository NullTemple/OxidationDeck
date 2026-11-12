#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>
#include <atomic>

/**
 * Stage 1: The 11 Harmonic Line Stage Engines
 * Features:
 *   - Continuous Pre-Emphasis Tilt Shelf filter (1 kHz pivot)
 *   - 11 Physical Saturation Kernels (Tubes, Cores, Solid-State, Exotics)
 *   - Clickless crossfade when switching circuit flavors on the fly
 *   - Per-sample parameter smoothing (Drive, Bias, Mix, Auto-Gain, Bypass)
 *   - Operating point Bias & Asymmetry shaper (Even vs Odd balance)
 *   - 5 Hz Sub-Audio DC Servo Highpass
 *   - Live Harmonic Telemetry for CRT Spectrum & Transfer Scope
 */
class PreampStage
{
public:
    PreampStage() = default;

    void prepare(double sampleRate, int /*samplesPerBlock*/)
    {
        setSampleRate(sampleRate);
        reset();
    }

    void setSampleRate(double sampleRate) noexcept
    {
        currentSampleRate = std::max(1000.0, sampleRate);
        smoothCoeff = 1.0f - std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.005)); // 5ms smoothing
        flavorFadeStep = 1.0f / std::max(32.0f, static_cast<float>(currentSampleRate * 0.008)); // 8ms crossfade
        currentTiltDb = -999.0f;
        updateCoefficients(targetTiltDb);
    }

    void reset() noexcept
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlockerX1[ch] = 0.0f;
            dcBlockerY1[ch] = 0.0f;
            ironLowState[ch] = 0.0f;
            ironHystState[ch] = 0.0f;
            starvedBias[ch] = 0.0f;
            steelInductorState[ch] = 0.0f;
            steelResState[ch] = 0.0f;

            tiltX1[ch] = 0.0f;
            tiltY1[ch] = 0.0f;
        }
        harmonicEnergy.store(0.0f, std::memory_order_relaxed);
    }

    void process(juce::dsp::AudioBlock<float>& block,
                 int flavorIndex,
                 float driveDb,
                 float biasPercent,
                 float preTiltDb,
                 float mixPercent,
                 bool autoGainEnabled,
                 bool bypassed = false) noexcept
    {
        const int numChannels = static_cast<int>(block.getNumChannels());
        const int numSamples = static_cast<int>(block.getNumSamples());

        if (numChannels == 0 || numSamples == 0)
            return;

        flavorIndex = juce::jlimit(0, 10, flavorIndex);
        if (flavorIndex != activeFlavor && flavorFade >= 1.0f)
        {
            prevFlavor = activeFlavor;
            activeFlavor = flavorIndex;
            flavorFade = 0.0f;
        }

        // 1. Target Drive & Compensation Gains
        float targetInGain = std::pow(10.0f, driveDb * 0.05f);
        float targetAutoComp = autoGainEnabled ? (1.0f / std::pow(std::max(1.0f, targetInGain), 0.60f)) : 1.0f;
        float targetBias = (biasPercent - 50.0f) * 0.01f * 0.35f; // Asymmetry shift [-0.175, +0.175]
        float targetMix = juce::jlimit(0.0f, 1.0f, mixPercent * 0.01f);
        float targetActive = bypassed ? 0.0f : 1.0f;

        // Update Tilt filter if tilt parameter changed
        targetTiltDb = preTiltDb;
        updateCoefficients(preTiltDb);

        // Precompute DC servo pole (5 Hz highpass) and LF transformer crossover (120 Hz)
        float dcR = 1.0f - static_cast<float>(2.0 * juce::MathConstants<double>::pi * 5.0 / currentSampleRate);
        dcR = juce::jlimit(0.99f, 0.99995f, dcR);

        ironAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * 120.0f / static_cast<float>(currentSampleRate));

        float maxHarmBlock = 0.0f;
        float* chPtr[2] = {
            block.getChannelPointer(0),
            (numChannels > 1) ? block.getChannelPointer(1) : nullptr
        };
        const int chCount = std::min(2, numChannels);

        for (int i = 0; i < numSamples; ++i)
        {
            // Per-sample parameter smoothing (eliminates zipper noise & clicks)
            smoothInGain   += smoothCoeff * (targetInGain - smoothInGain);
            smoothAutoComp += smoothCoeff * (targetAutoComp - smoothAutoComp);
            smoothBias     += smoothCoeff * (targetBias - smoothBias);
            smoothMix      += smoothCoeff * (targetMix - smoothMix);
            smoothActive   += smoothCoeff * (targetActive - smoothActive);

            if (flavorFade < 1.0f)
                flavorFade = std::min(1.0f, flavorFade + flavorFadeStep);

            for (int ch = 0; ch < chCount; ++ch)
            {
                float xDry = chPtr[ch][i];
                if (!std::isfinite(xDry))
                    xDry = 0.0f;

                // A. Pre-Emphasis Spectral Tilt (pivoting at 1 kHz)
                float xTilted = processTilt(xDry, ch);

                // B. Input Drive & Bias Asymmetry (zero-thump quiescent offset cancellation)
                float xDriven = (xTilted * smoothInGain) + smoothBias;

                // C. Nonlinear Kernel f(x) with clickless flavor crossfade
                float ySat = evaluateKernel(xDriven, activeFlavor, ch) - evaluateBiasNull(smoothBias, activeFlavor);
                if (flavorFade < 1.0f && prevFlavor != activeFlavor)
                {
                    float yPrev = evaluateKernel(xDriven, prevFlavor, ch) - evaluateBiasNull(smoothBias, prevFlavor);
                    ySat = yPrev + flavorFade * (ySat - yPrev);
                }

                // D. 5 Hz DC Servo Highpass Filter
                float yClean = ySat - dcBlockerX1[ch] + dcR * dcBlockerY1[ch];
                dcBlockerX1[ch] = ySat;
                dcBlockerY1[ch] = yClean;

                // E. Auto-Gain Compensation
                float yWet = yClean * smoothAutoComp;

                // F. Dry / Wet Parallel Mix & Clickless Bypass Crossfade
                float yParallel = (1.0f - smoothMix) * xDry + smoothMix * yWet;
                float yFinal = xDry + smoothActive * (yParallel - xDry);

                if (!std::isfinite(yFinal))
                    yFinal = 0.0f;

                chPtr[ch][i] = yFinal;

                float harmDiff = std::abs(yWet - xDry) + std::abs(yWet) * 0.35f;
                maxHarmBlock = std::max(maxHarmBlock, harmDiff);
            }
        }

        // Update smoothed harmonic telemetry for UI CRT screen
        float prevHarm = harmonicEnergy.load(std::memory_order_relaxed);
        float nextHarm = (maxHarmBlock > prevHarm)
            ? (prevHarm + 0.35f * (maxHarmBlock - prevHarm))
            : (prevHarm + 0.08f * (maxHarmBlock - prevHarm));
        harmonicEnergy.store(bypassed ? 0.0f : juce::jlimit(0.0f, 1.5f, nextHarm), std::memory_order_relaxed);
    }

    float getHarmonicActivity() const noexcept
    {
        return harmonicEnergy.load(std::memory_order_relaxed);
    }

private:
    double currentSampleRate = 44100.0;
    float smoothCoeff = 0.005f;
    float ironAlpha = 0.99f;

    // Smoothed parameters
    float smoothInGain   = 1.4125f;
    float smoothAutoComp = 0.81f;
    float smoothBias     = 0.05f;
    float smoothMix      = 1.0f;
    float smoothActive   = 1.0f;

    // Flavor crossfade
    int activeFlavor = 0;
    int prevFlavor   = 0;
    float flavorFade = 1.0f;
    float flavorFadeStep = 0.002f;

    // Tilt Filter Coefficients
    float tiltB0 = 1.0f;
    float tiltB1 = 0.0f;
    float tiltA1 = 0.0f;
    float currentTiltDb = -999.0f;
    float targetTiltDb = 0.0f;

    // State Variables per Channel (Stereo)
    float tiltX1[2] { 0.0f, 0.0f };
    float tiltY1[2] { 0.0f, 0.0f };

    float dcBlockerX1[2] { 0.0f, 0.0f };
    float dcBlockerY1[2] { 0.0f, 0.0f };

    // Kernel-Specific States
    float ironLowState[2]      { 0.0f, 0.0f };
    float ironHystState[2]     { 0.0f, 0.0f };
    float starvedBias[2]       { 0.0f, 0.0f };
    float steelInductorState[2]{ 0.0f, 0.0f };
    float steelResState[2]     { 0.0f, 0.0f };

    std::atomic<float> harmonicEnergy { 0.0f };

    void updateCoefficients(float tiltDb) noexcept
    {
        if (std::abs(tiltDb - currentTiltDb) < 0.01f)
            return;

        currentTiltDb = tiltDb;

        if (std::abs(tiltDb) < 0.05f)
        {
            tiltB0 = 1.0f;
            tiltB1 = 0.0f;
            tiltA1 = 0.0f;
            return;
        }

        // 1st-order shelf tilt around 1 kHz
        double gainLinear = std::pow(10.0, static_cast<double>(tiltDb) / 20.0);
        double sqrtGain = std::sqrt(gainLinear);

        double w0 = 2.0 * juce::MathConstants<double>::pi * 1000.0;
        double K = std::tan(w0 / (2.0 * currentSampleRate));

        // Bilinear transform formulation
        double norm = 1.0 / (K + sqrtGain);
        tiltB0 = static_cast<float>((K * gainLinear + sqrtGain) * norm);
        tiltB1 = static_cast<float>((K * gainLinear - sqrtGain) * norm);
        tiltA1 = static_cast<float>((K - sqrtGain) * norm);
    }

    inline float processTilt(float in, int ch) noexcept
    {
        if (std::abs(currentTiltDb) < 0.05f)
            return in;

        float out = tiltB0 * in + tiltB1 * tiltX1[ch] - tiltA1 * tiltY1[ch];
        tiltX1[ch] = in;
        tiltY1[ch] = out;
        return out;
    }

    inline float evaluateKernel(float x, int flavorIndex, int ch) noexcept
    {
        // Guard against extreme inputs before nonlinear evaluation
        x = juce::jlimit(-12.0f, 12.0f, x);

        switch (flavorIndex)
        {
            // ==========================================
            // Family A: Vacuum Tubes (The Glass Chamber)
            // ==========================================
            case 0: // 01: Triode Class-A (12AX7 / 300B)
            {
                // Dominant 2nd harmonic bloom (Child-Langmuir 3/2 power law with soft grid-current ceiling)
                float absX = std::abs(x);
                float num = x + 0.32f * (x * x) / (1.0f + 0.35f * absX);
                float den = 1.0f + 0.28f * absX;
                return std::tanh(num / den);
            }

            case 1: // 02: Pentode Push-Pull (EL34 / EF86)
            {
                // Pure odd harmonics (H3, H5) from screen grid isolation and push-pull cancellation
                float sgn = (x >= 0.0f) ? 1.0f : -1.0f;
                float absX = std::abs(x);
                float p3 = absX * absX * absX;
                return sgn * (absX / std::cbrt(1.0f + p3));
            }

            case 2: // 03: Starved-Plate Triode (Low Anode Voltage)
            {
                // Dynamic cathode bias drift and asymmetric soft clipping
                float lambda = 1.0f - std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.045));
                float excess = std::max(0.0f, x - 0.25f);
                starvedBias[ch] += lambda * (excess - starvedBias[ch]);
                return std::tanh(1.55f * (x - starvedBias[ch]));
            }

            // ==========================================
            // Family B: Transformers & Inductors (Cores)
            // ==========================================
            case 3: // 04: Iron Core Transformer (Neve Marinair)
            {
                // Unconditionally stable frequency-split Faraday core saturation (<120 Hz girth + linear HF coupling)
                ironLowState[ch] = ironAlpha * ironLowState[ch] + (1.0f - ironAlpha) * x;
                float xLow  = ironLowState[ch];
                float xHigh = x - xLow;

                // Magnetic core saturation + subtle remanence loop on LF
                float coreSat = (2.0f / juce::MathConstants<float>::pi) * std::atan(1.85f * xLow + 0.15f * ironHystState[ch]);
                ironHystState[ch] = 0.92f * ironHystState[ch] + 0.08f * coreSat;

                return std::tanh(1.15f * coreSat + 0.92f * xHigh);
            }

            case 4: // 05: Nickel Core Transformer (API 2520)
            {
                // High initial permeability, tight punchy transient knee, fast upper-harmonic snap
                float absX = std::abs(x);
                float knee = 0.85f;
                float compression = 1.0f / (1.0f + std::exp(-8.0f * (absX - knee)));
                float shaped = x * (1.0f - 0.30f * compression);
                return std::tanh(1.12f * shaped);
            }

            case 5: // 06: Steel Core / Passive Inductor (Pultec EQP-1A)
            {
                // Unconditionally stable exponential inductor core integrator (160 Hz warm body)
                float alphaL = 1.0f - ironAlpha;
                steelInductorState[ch] += alphaL * (x - steelInductorState[ch]);
                float indSat = std::tanh(1.35f * steelInductorState[ch]);
                return std::tanh(0.75f * x + 0.35f * indSat);
            }

            // ==========================================
            // Family C: Semiconductors (Solid-State)
            // ==========================================
            case 6: // 07: Class-A Discrete Silicon (API / Neve 1272 BJT)
            {
                // Classic 3rd-order polynomial soft-clipper with smooth saturation ceiling
                float xs = juce::jlimit(-1.5f, 1.5f, x * 0.85f);
                if (std::abs(xs) < 1.0f)
                    return xs - (xs * xs * xs) * (1.0f / 3.0f);
                return (2.0f / 3.0f) * ((xs >= 0.0f) ? 1.0f : -1.0f);
            }

            case 7: // 08: Germanium Transistor (Chandler TG / EMI)
            {
                // Low forward-voltage knee (0.28V), velvety warm saturation curve
                float absX = std::abs(x);
                float sgn = (x >= 0.0f) ? 1.0f : -1.0f;
                float geSoft = 1.0f - std::exp(-absX / 0.32f);
                return sgn * (0.32f * geSoft + 0.68f * std::tanh(absX * 0.95f));
            }

            case 8: // 09: FET Line Driver (Field Effect / 1176)
            {
                // Shockley square-law conduction with tube-like even harmonics
                float xClamped = std::max(-1.45f, x);
                float y = xClamped + 0.20f * (xClamped * xClamped) / (1.0f + 0.4f * std::abs(xClamped));
                return std::tanh(y);
            }

            // ==========================================
            // Family D: Mauve Zone Exotics
            // ==========================================
            case 9: // 10: Abbey Road Zener Diode Clipper (TG12413)
            {
                // Dual back-to-back Zener diode voltage clamp
                const float Vz = 0.82f;
                const float Vf = 0.18f;
                float absX = std::abs(x);
                float sgn = (x >= 0.0f) ? 1.0f : -1.0f;
                return Vz * std::tanh(x / Vz) + sgn * Vf * (1.0f - std::exp(-absX / Vf));
            }

            case 10: // 11: Oxide Rot & Flux Breakdown (Mauve Zone Signature)
            default:
            {
                // Chebyshev 3rd-order harmonic wavefolding with smooth saturation clamp
                float xc = std::tanh(x * 0.75f);
                float T3 = 4.0f * (xc * xc * xc) - 3.0f * xc;
                return 0.78f * std::tanh(x) - 0.22f * std::tanh(T3);
            }
        }
    }

    static inline float evaluateBiasNull(float b, int flavorIndex) noexcept
    {
        if (std::abs(b) < 1e-5f)
            return 0.0f;

        float absB = std::abs(b);
        float sgn  = (b >= 0.0f) ? 1.0f : -1.0f;
        switch (flavorIndex)
        {
            case 0:  return std::tanh((b + 0.32f * b * b / (1.0f + 0.35f * absB)) / (1.0f + 0.28f * absB));
            case 1:  return sgn * (absB / std::cbrt(1.0f + absB * absB * absB));
            case 2:  return std::tanh(1.55f * b);
            case 3:  return std::tanh(1.15f * (2.0f / juce::MathConstants<float>::pi) * std::atan(1.85f * b));
            case 4:  return std::tanh(1.12f * b * (1.0f - 0.30f / (1.0f + std::exp(-8.0f * (absB - 0.85f)))));
            case 5:  return std::tanh(0.75f * b + 0.35f * std::tanh(1.35f * b));
            case 6:  { float xs = juce::jlimit(-1.5f, 1.5f, b * 0.85f); return xs - (xs * xs * xs) * (1.0f / 3.0f); }
            case 7:  return sgn * (0.32f * (1.0f - std::exp(-absB / 0.32f)) + 0.68f * std::tanh(absB * 0.95f));
            case 8:  return std::tanh(b + 0.20f * b * b / (1.0f + 0.4f * absB));
            case 9:  return 0.82f * std::tanh(b / 0.82f) + sgn * 0.18f * (1.0f - std::exp(-absB / 0.18f));
            default: { float xc = std::tanh(b * 0.75f); float T3 = 4.0f * xc * xc * xc - 3.0f * xc; return 0.78f * std::tanh(b) - 0.22f * std::tanh(T3); }
        }
    }
};
