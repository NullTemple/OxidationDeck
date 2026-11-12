#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "UI/MauveLookAndFeel.h"
#include "UI/Deck1Preamp.h"
#include "UI/Deck2Dynamics.h"
#include "UI/Deck3Tape.h"
#include "UI/Deck4Limiter.h"

class OxidationDeckAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer,
                                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit OxidationDeckAudioProcessorEditor(OxidationDeckAudioProcessor&);
    ~OxidationDeckAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int kBaseWidth  = 1220;
    static constexpr int kBaseHeight = 570;

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncHeaderControls();
    void swapDeckAtSlot(int slotIndex);
    void applyFactoryPreset(int presetId);

    OxidationDeckAudioProcessor& processorRef;
    MauveLookAndFeel lnf;
    juce::Component canvas;

    // Header Controls
    juce::ComboBox chainBox;
    juce::ComboBox presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chainAttach;

    juce::TextButton os2xBtn;
    juce::TextButton os4xBtn;
    juce::TextButton os8xBtn;

    juce::TextButton stereoBtn;
    juce::TextButton msBtn;

    // The 4 Deck Panels
    Deck1Preamp   deck1;
    Deck2Dynamics deck2;
    Deck3Tape     deck3;
    Deck4Limiter  deck4;

    // Live Ballistics Smoothing & Peak Hold
    float smoothInL = 0.0f;
    float smoothInR = 0.0f;
    float smoothOutL = 0.0f;
    float smoothOutR = 0.0f;
    float peakHoldInL = 0.0f;
    float peakHoldInR = 0.0f;
    float peakHoldOutL = 0.0f;
    float peakHoldOutR = 0.0f;
    float smoothCorr = 1.0f;

    void drawChassisHeader(juce::Graphics& g, juce::Rectangle<int> area);
    void drawChassisFooter(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OxidationDeckAudioProcessorEditor)
};
