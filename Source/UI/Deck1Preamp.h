#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "MauveTheme.h"
#include "LabeledKnob.h"
#include "CrtDisplays.h"
#include "../Parameters.h"

class Deck1Preamp : public juce::Component
{
public:
    explicit Deck1Preamp(juce::AudioProcessorValueTreeState& apvts);
    ~Deck1Preamp() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void updateHarmonicTelemetry(float activity) { harmScreen.setLiveActivity(activity); }
    void setSlotNumber(int slot)
    {
        if (slotNumber != slot)
        {
            slotNumber = slot;
            repaint();
        }
    }

    std::function<void()> onSwapRight;

private:
    int slotNumber = 1;

    juce::ComboBox flavorBox;
    Deck1HarmonicScreen harmScreen;

    LabeledKnob driveKnob;
    LabeledKnob biasKnob;
    LabeledKnob pretiltKnob;
    LabeledKnob parallelKnob;

    juce::TextButton autoGainBtn;
    juce::TextButton bypassBtn;
    juce::TextButton swapBtn;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> flavorAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   biasAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   pretiltAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   parallelAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   autoGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   bypassAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck1Preamp)
};
