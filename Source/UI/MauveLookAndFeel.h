#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MauveTheme.h"

class MauveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MauveLookAndFeel();
    ~MauveLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;

    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label) override;
    void drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& sectionName) override;
    juce::Font getPopupMenuFont() override;

    static void drawChassisScrew(juce::Graphics& g, float cx, float cy, float diameter = 12.0f);
};

