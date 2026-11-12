#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "MauveTheme.h"
#include "LabeledKnob.h"
#include "CrtDisplays.h"
#include "../Parameters.h"

class Deck2Dynamics : public juce::Component,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit Deck2Dynamics(juce::AudioProcessorValueTreeState& apvts);
    ~Deck2Dynamics() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void updateVUMeter(float grL, float grR) { vuMeter.setGR(grL, grR); }
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
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void syncModeAndHpf();

    int slotNumber = 2;

    juce::TextButton vcaBtn;
    juce::TextButton variMuBtn;
    StereoVUMeter vuMeter;

    LabeledKnob threshKnob;
    LabeledKnob ratioKnob;
    LabeledKnob attackKnob;
    LabeledKnob releaseKnob;

    juce::TextButton scHpfBtn;
    juce::TextButton listenBtn;
    juce::TextButton bypassBtn;
    juce::TextButton swapBtn;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    juce::AudioProcessorValueTreeState& apvtsRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck2Dynamics)
};
