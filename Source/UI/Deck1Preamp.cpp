#include "Deck1Preamp.h"

Deck1Preamp::Deck1Preamp(juce::AudioProcessorValueTreeState& apvts)
    : driveKnob("DRIVE"),
      biasKnob("BIAS"),
      pretiltKnob("PRE-TILT"),
      parallelKnob("PARALLEL")
{
    // Setup Flavor ComboBox with Categories & Sub-Flavors
    flavorBox.clear();
    flavorBox.addSectionHeading("TUBES (GLASS)");
    flavorBox.addItem("TRIODE CLASS-A (12AX7)", 1);
    flavorBox.addItem("PENTODE PUSH-PULL (EL34)", 2);
    flavorBox.addItem("STARVED TRIODE", 3);

    flavorBox.addSectionHeading("TRANSFORMERS (CORE)");
    flavorBox.addItem("IRON CORE (NEVE)", 4);
    flavorBox.addItem("NICKEL CORE (API)", 5);
    flavorBox.addItem("STEEL INDUCTOR (PULTEC)", 6);

    flavorBox.addSectionHeading("SOLID-STATE");
    flavorBox.addItem("CLASS-A SILICON", 7);
    flavorBox.addItem("GERMANIUM (TG)", 8);
    flavorBox.addItem("FET DRIVER (1176)", 9);

    flavorBox.addSectionHeading("EXOTICS");
    flavorBox.addItem("ZENER SOFT-CLIPPER", 10);
    flavorBox.addItem("OXIDE ROT & BREAKDOWN", 11);

    addAndMakeVisible(flavorBox);

    auto updateHarmScreen = [this]()
    {
        int idx = flavorBox.getSelectedId() - 1;
        const juce::String tags[] = {
            "2ND HARMONIC BLOOM",
            "3RD HARMONIC PUNCH",
            "DARK TUBE ASYMMETRY",
            "SUB-BASS FLUX GIRTH",
            "CLEAN TRANSIENT SNAP",
            "INDUCTOR RESONANCE",
            "CLASS-A BJT EDGE",
            "VELVETY TG WARMTH",
            "1176 FAST TUBE SHEEN",
            "ABBEY ROAD SMOOTH",
            "CHAOTIC FLUX DECAY"
        };
        const float profiles[][3] = {
            { 0.85f, 0.65f, 0.15f }, // 01 Triode: High H2 bloom
            { 0.85f, 0.12f, 0.70f }, // 02 Pentode: High H3 punch
            { 0.90f, 0.75f, 0.40f }, // 03 Starved: Asymmetric drift
            { 0.80f, 0.20f, 0.65f }, // 04 Iron: LF 3rd flux girth
            { 0.85f, 0.35f, 0.45f }, // 05 Nickel: Clean transient snap
            { 0.75f, 0.40f, 0.55f }, // 06 Steel: Inductor resonance
            { 0.85f, 0.18f, 0.78f }, // 07 BJT: Hard 3rd bite
            { 0.90f, 0.45f, 0.45f }, // 08 Germanium: Velvety warmth
            { 0.85f, 0.62f, 0.30f }, // 09 FET: Tube sheen
            { 0.85f, 0.38f, 0.58f }, // 10 Zener: Symmetrical clamp
            { 0.95f, 0.82f, 0.75f }  // 11 Oxide Rot: Wavefolded decay
        };

        if (idx >= 0 && idx < 11)
        {
            float driveVal = static_cast<float>(driveKnob.getSlider().getValue());
            float biasNorm = static_cast<float>(biasKnob.getSlider().getValue()) / 100.0f;
            float driveScale = juce::jlimit(0.3f, 1.4f, 1.0f + driveVal * 0.02f);
            float evenBoost  = 0.75f + 0.50f * biasNorm;
            harmScreen.setProfile(tags[idx],
                                  juce::jlimit(0.1f, 1.0f, profiles[idx][0] * driveScale),
                                  juce::jlimit(0.05f, 1.0f, profiles[idx][1] * driveScale * evenBoost),
                                  juce::jlimit(0.05f, 1.0f, profiles[idx][2] * driveScale),
                                  idx,
                                  driveVal / 24.0f,
                                  biasNorm);
        }
    };

    flavorBox.onChange = updateHarmScreen;
    addAndMakeVisible(harmScreen);

    // Knobs (Drive in positive Decibels)
    driveKnob.setValueFormat([](double v) {
        if (v < 0.05) return juce::String("0.0 dB");
        return "+" + juce::String(v, 1) + " dB";
    });
    addAndMakeVisible(driveKnob);

    biasKnob.setValueFormat([](double v) { return juce::String((int)v) + "% EVEN"; });
    addAndMakeVisible(biasKnob);

    pretiltKnob.setValueFormat([](double v) {
        if (std::abs(v) < 0.05) return juce::String("FLAT");
        return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB";
    });
    addAndMakeVisible(pretiltKnob);

    parallelKnob.setValueFormat([](double v) { return juce::String((int)v) + "%"; });
    addAndMakeVisible(parallelKnob);

    // Auto-Gain Button
    autoGainBtn.setButtonText("ON");
    autoGainBtn.setClickingTogglesState(true);
    addAndMakeVisible(autoGainBtn);

    // Swap Deck Order Button
    swapBtn.setButtonText(juce::String::fromUTF8("\xe2\x87\x84"));
    swapBtn.setTooltip("Swap Preamp position in Signal Chain");
    swapBtn.onClick = [this]() { if (onSwapRight) onSwapRight(); };
    addAndMakeVisible(swapBtn);

    // Bypass Button in Header
    bypassBtn.setButtonText("BYPASS");
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setTooltip("Bypass Preamp / Line Stage");
    addAndMakeVisible(bypassBtn);

    // Attachments
    flavorAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, IDs::flavor.getParamID(), flavorBox);
    driveAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::drive.getParamID(), driveKnob.getSlider());
    biasAttach     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::bias.getParamID(), biasKnob.getSlider());
    pretiltAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::preTilt.getParamID(), pretiltKnob.getSlider());
    parallelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::parallelLine.getParamID(), parallelKnob.getSlider());
    autoGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::autoGain.getParamID(), autoGainBtn);
    bypassAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::deck1Bypass.getParamID(), bypassBtn);

    // Initial sync & drive/bias response + double-click reset values
    driveKnob.getSlider().setDoubleClickReturnValue(true, 4.5);
    biasKnob.getSlider().setDoubleClickReturnValue(true, 62.0);
    pretiltKnob.getSlider().setDoubleClickReturnValue(true, 0.0);
    parallelKnob.getSlider().setDoubleClickReturnValue(true, 100.0);

    driveKnob.getSlider().onValueChange = [this, updateHarmScreen]() {
        updateHarmScreen();
        driveKnob.repaint();
    };
    biasKnob.getSlider().onValueChange = [this, updateHarmScreen]() {
        updateHarmScreen();
        biasKnob.repaint();
    };
    updateHarmScreen();
}

void Deck1Preamp::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header area (20px)
    auto headerArea = area.removeFromTop(20);
    bypassBtn.setBounds(headerArea.removeFromRight(50).reduced(0, 1));
    headerArea.removeFromRight(3);
    swapBtn.setBounds(headerArea.removeFromRight(24).reduced(0, 1));
    area.removeFromTop(6);

    // Circuit ComboBox (28px)
    flavorBox.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);

    // CRT Screen (42px)
    harmScreen.setBounds(area.removeFromTop(42));
    area.removeFromTop(8);

    // Footer at bottom (24px)
    auto footerArea = area.removeFromBottom(24);
    autoGainBtn.setBounds(footerArea.removeFromRight(40).withSizeKeepingCentre(36, 20));

    // 2x2 Knobs grid
    int colW = area.getWidth() / 2;
    int rowH = area.getHeight() / 2;

    driveKnob.setBounds(area.getX(), area.getY(), colW, rowH);
    biasKnob.setBounds(area.getX() + colW, area.getY(), colW, rowH);
    pretiltKnob.setBounds(area.getX(), area.getY() + rowH, colW, rowH);
    parallelKnob.setBounds(area.getX() + colW, area.getY() + rowH, colW, rowH);
}

void Deck1Preamp::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Drop shadow under deck card
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 4.0f);

    // 2. Deck Console Gradient Surface
    juce::ColourGradient bg(MauveTheme::deckConsole1, 0.0f, 0.0f,
                            MauveTheme::deckConsole2, 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 4.0f);

    // 3. Top edge highlight line
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.08f));
    g.drawLine(bounds.getX() + 3.0f, bounds.getY() + 1.0f, bounds.getRight() - 3.0f, bounds.getY() + 1.0f, 1.0f);

    // 4. Deck Border
    g.setColour(MauveTheme::deckConsoleBrd);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Header bar
    auto headerArea = bounds.reduced(8.0f, 6.0f).removeFromTop(18.0f);

    // Amber Tally Dot (Glows amber when active, dims when bypassed)
    bool isBypassed = bypassBtn.getToggleState();
    if (!isBypassed)
    {
        g.setColour(MauveTheme::amber.withAlpha(0.35f));
        g.fillEllipse(headerArea.getX() - 1.5f, headerArea.getCentreY() - 5.0f, 10.0f, 10.0f);
    }
    g.setColour(isBypassed ? juce::Colour::fromRGB(65, 42, 85) : MauveTheme::amberBright);
    g.fillEllipse(headerArea.getX(), headerArea.getCentreY() - 3.5f, 7.0f, 7.0f);

    // Deck Title with Dynamic Slot Number
    g.setColour(isBypassed ? MauveTheme::silkscreen : MauveTheme::textBright);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    juce::String prefix = "0" + juce::String(slotNumber) + juce::String::fromUTF8(" \xc2\xb7 PREAMP / LINE");
    g.drawText(prefix, headerArea.withTrimmedLeft(12.0f), juce::Justification::centredLeft, false);

    // Header separator line
    g.setColour(juce::Colour::fromRGB(56, 38, 78));
    g.drawLine(bounds.getX() + 8.0f, headerArea.getBottom() + 3.0f, bounds.getRight() - 8.0f, headerArea.getBottom() + 3.0f, 1.0f);

    // Footer separator line
    float footY = bounds.getBottom() - 32.0f;
    g.drawLine(bounds.getX() + 8.0f, footY, bounds.getRight() - 8.0f, footY, 1.0f);

    // Footer label
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText("AUTO-GAIN", juce::Rectangle<float>(bounds.getX() + 8.0f, footY + 4.0f, 80.0f, 22.0f), juce::Justification::centredLeft, false);
}
