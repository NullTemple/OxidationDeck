#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "DSP/PreampStage.h"
#include "DSP/DynamicsStage.h"
#include "DSP/TapeStage.h"
#include "DSP/LimiterStage.h"
#include <atomic>

class OxidationDeckAudioProcessor : public juce::AudioProcessor
{
public:
    OxidationDeckAudioProcessor();
    ~OxidationDeckAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    void applyFactoryPreset(int programIndex);

    // Live Telemetry Getters (Thread-Safe Lock-Free)
    float getInputPeakL() const noexcept    { return inPeakL.load(std::memory_order_relaxed); }
    float getInputPeakR() const noexcept    { return inPeakR.load(std::memory_order_relaxed); }
    float getOutputPeakL() const noexcept   { return outPeakL.load(std::memory_order_relaxed); }
    float getOutputPeakR() const noexcept   { return outPeakR.load(std::memory_order_relaxed); }
    float getCorrelation() const noexcept   { return correlationVal.load(std::memory_order_relaxed); }
    float getGainReductionL() const noexcept { return grL.load(std::memory_order_relaxed); }
    float getGainReductionR() const noexcept { return grR.load(std::memory_order_relaxed); }
    float getPreampHarmonicActivity() const noexcept { return preampStage.getHarmonicActivity(); }
    float getTapeFluxIntensity() const noexcept { return tapeStage.getFluxIntensity(); }
    float getLimiterGainReduction() const noexcept { return limiterStage.getGainReductionDb(); }
    int   getLimiterClipCount() const noexcept     { return limiterStage.getClipCount(); }
    void  resetLimiterClips() noexcept             { limiterStage.resetClips(); }
    float getLimiterRadarNorm() const noexcept     { return limiterStage.getRadarNorm(); }
    float getLimiterIntegratedLufs() const noexcept { return limiterStage.getIntegratedLufs(); }
    float getLimiterTruePeak() const noexcept      { return limiterStage.getTruePeakDb(); }
    float getLimiterShortTermLufs() const noexcept { return limiterStage.getShortTermLufs(); }
    float getLimiterLra() const noexcept           { return limiterStage.getLra(); }

private:
    juce::AudioProcessorValueTreeState apvts;

    // Multirate Oversampling Engine (2x, 4x, 8x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[3];
    int currentOversampleIndex = 1; // Default: 4x
    int currentChainOrder = 0;
    int currentProgramIndex = 0;
    float chainTransitionGain = 1.0f;
    double baseSampleRate = 44100.0;
    int baseBlockSize = 512;

    // Deck Stages
    PreampStage preampStage;
    DynamicsStage dynamicsStage;
    TapeStage tapeStage;
    LimiterStage limiterStage;

    // Live Telemetry Atomicals
    std::atomic<float> inPeakL { 0.0f };
    std::atomic<float> inPeakR { 0.0f };
    std::atomic<float> outPeakL { 0.0f };
    std::atomic<float> outPeakR { 0.0f };
    std::atomic<float> correlationVal { 1.0f };
    std::atomic<float> grL { 0.0f };
    std::atomic<float> grR { 0.0f };

    // Latency-matched bypass delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> bypassDelay { 8192 };

    void updateOversamplingChoice();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OxidationDeckAudioProcessor)
};
