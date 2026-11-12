#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MauveTheme.h"

class LabeledKnob : public juce::Component
{
public:
    LabeledKnob(const juce::String& labelText, bool isLarge = false, bool isFluted = false)
        : label(labelText), large(isLarge)
    {
        if (isLarge)
            slider.getProperties().set("large", true);
        if (isFluted)
            slider.getProperties().set("fluted", true);

        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters(juce::degreesToRadians(-135.0f), juce::degreesToRadians(135.0f), true);
        slider.setMouseDragSensitivity(135);
        slider.setVelocityModeParameters(1.0, 1, 0.05, true, juce::ModifierKeys::ctrlAltCommandModifiers);
        addAndMakeVisible(slider);

        slider.onValueChange = [this]() { repaint(); };
    }

    ~LabeledKnob() override = default;

    juce::Slider& getSlider() { return slider; }

    void setValueFormat(std::function<juce::String(double)> formatter)
    {
        customFormatter = std::move(formatter);
        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        // Reserve 32px at the bottom for label + value
        auto knobArea = bounds.removeFromTop(juce::jmax(10, bounds.getHeight() - 32));

        // BIG, prominent hardware dials that eat the vertical space
        int targetDim = large ? 90 : 80;
        int maxDim = juce::jmin(knobArea.getWidth() - 2, knobArea.getHeight());
        int knobDim = juce::jmin(targetDim, maxDim);

        slider.setBounds(knobArea.withSizeKeepingCentre(knobDim, knobDim));
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto textArea = bounds.removeFromBottom(32);

        // Line 1: Parameter Label
        g.setColour(MauveTheme::silkscreen);
        g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        g.drawText(label, textArea.removeFromTop(15).toFloat(), juce::Justification::centred, false);

        // Line 2: Formatted Value
        juce::String valStr;
        if (customFormatter)
            valStr = customFormatter(slider.getValue());
        else
            valStr = juce::String(slider.getValue(), 1) + " " + slider.getTextValueSuffix();

        g.setColour(MauveTheme::amberBright);
        g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        g.drawText(valStr.trim(), textArea.toFloat(), juce::Justification::centred, false);
    }

private:
    juce::Slider slider;
    juce::String label;
    bool large;
    std::function<juce::String(double)> customFormatter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledKnob)
};
