#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "MauveTheme.h"
#include "TapeTransportComponent.h"
#include "LabeledKnob.h"
#include "../Parameters.h"

class Deck3Tape : public juce::Component, private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit Deck3Tape(juce::AudioProcessorValueTreeState& apvts);
    ~Deck3Tape() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void updateFluxTelemetry(float flux) { transport.setFluxIntensity(flux); }
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
    void updateSpeedDisplay(int speedIndex);

    int slotNumber = 3;
    juce::AudioProcessorValueTreeState& apvtsRef;

    TapeTransportComponent transport;

    // Interactive Mechanical Odometer Button
    juce::TextButton speedBtn;
    juce::TextButton bypassBtn;
    juce::TextButton swapBtn;

    LabeledKnob satKnob;
    LabeledKnob biasCalKnob;
    LabeledKnob flutterKnob;
    LabeledKnob wearKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> satAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> biasCalAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> flutterAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wearAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    int currentSpeed = 1; // 0 = 15 IPS, 1 = 30 IPS

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck3Tape)
};
