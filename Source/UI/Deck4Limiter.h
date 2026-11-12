#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "MauveTheme.h"
#include "LabeledKnob.h"
#include "CrtDisplays.h"
#include "../Parameters.h"

class Deck4Limiter : public juce::Component
{
public:
    explicit Deck4Limiter(juce::AudioProcessorValueTreeState& apvts);
    ~Deck4Limiter() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void updateTelemetry(float grDb, int clips, float lufsInt, float tpMax, float lufsSt, float lra);

    std::function<void()> onResetClips;

private:
    Deck4LoudnessRadar radar;

    LabeledKnob ceilingKnob;
    LabeledKnob inGainKnob;
    LabeledKnob lookaheadKnob;
    LabeledKnob releaseKnob;

    juce::TextButton bypassBtn;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ceilingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lookaheadAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;

    int currentClips = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Deck4Limiter)
};
