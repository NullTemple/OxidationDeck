#include "Deck3Tape.h"

Deck3Tape::Deck3Tape(juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef(apvts),
      satKnob("SATURATION", false, true),
      biasCalKnob("BIAS CAL", false, true),
      flutterKnob("WOW/FLUTTER", false, true),
      wearKnob("OXIDE WEAR", false, true)
{
    addAndMakeVisible(transport);

    // Interactive Mechanical Odometer Speed Button
    speedBtn.setButtonText("30.00 IPS");
    speedBtn.setTooltip("Click to toggle Tape Speed (15 IPS vs 30 IPS)");
    speedBtn.onClick = [this]()
    {
        if (auto* param = apvtsRef.getParameter(IDs::tapeSpeed.getParamID()))
        {
            float curVal = param->getValue();
            float nextVal = (curVal > 0.5f) ? 0.0f : 1.0f;
            param->setValueNotifyingHost(nextVal);
        }
    };
    addAndMakeVisible(speedBtn);

    // Knobs setup
    satKnob.setValueFormat([](double v) {
        return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB";
    });
    addAndMakeVisible(satKnob);

    biasCalKnob.setValueFormat([](double v) {
        return (v > 0 ? "+" : "") + juce::String(v, 1) + " dB";
    });
    addAndMakeVisible(biasCalKnob);

    flutterKnob.setValueFormat([](double v) { return juce::String((int)v) + "%"; });
    addAndMakeVisible(flutterKnob);

    wearKnob.setValueFormat([](double v) { return juce::String((int)v) + "%"; });
    addAndMakeVisible(wearKnob);

    // Attachments
    satAttach     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::tapeSat.getParamID(), satKnob.getSlider());
    biasCalAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::tapeBiasCal.getParamID(), biasCalKnob.getSlider());
    flutterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::tapeFlutter.getParamID(), flutterKnob.getSlider());
    wearAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::tapeWear.getParamID(), wearKnob.getSlider());

    satKnob.getSlider().setDoubleClickReturnValue(true, 3.8);
    biasCalKnob.getSlider().setDoubleClickReturnValue(true, 1.5);
    flutterKnob.getSlider().setDoubleClickReturnValue(true, 10.0);
    wearKnob.getSlider().setDoubleClickReturnValue(true, 14.0);

    // Swap Deck Order Button
    swapBtn.setButtonText(juce::String::fromUTF8("\xe2\x87\x84"));
    swapBtn.setTooltip("Swap Tape Machine position in Signal Chain");
    swapBtn.onClick = [this]() { if (onSwapRight) onSwapRight(); };
    addAndMakeVisible(swapBtn);

    // Bypass Button in Header
    bypassBtn.setButtonText("BYPASS");
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setTooltip("Bypass Oxidation Tape Machine");
    bypassBtn.onClick = [this]()
    {
        transport.setRunning(!bypassBtn.getToggleState());
        repaint();
    };
    addAndMakeVisible(bypassBtn);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::deck3Bypass.getParamID(), bypassBtn);
    transport.setRunning(!bypassBtn.getToggleState());

    // Register listener for tape speed
    apvtsRef.addParameterListener(IDs::tapeSpeed.getParamID(), this);

    if (auto* p = apvtsRef.getRawParameterValue(IDs::tapeSpeed.getParamID()))
    {
        updateSpeedDisplay(juce::roundToInt(p->load()));
    }
}

Deck3Tape::~Deck3Tape()
{
    apvtsRef.removeParameterListener(IDs::tapeSpeed.getParamID(), this);
}

void Deck3Tape::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == IDs::tapeSpeed.getParamID())
    {
        juce::Component::SafePointer<Deck3Tape> safeThis(this);
        juce::MessageManager::callAsync([safeThis, val = juce::roundToInt(newValue)]()
        {
            if (safeThis != nullptr)
                safeThis->updateSpeedDisplay(val);
        });
    }
}

void Deck3Tape::updateSpeedDisplay(int speedIndex)
{
    currentSpeed = juce::jlimit(0, 1, speedIndex);
    if (currentSpeed == 0)
    {
        speedBtn.setButtonText("15.00 IPS");
        transport.setSpeedIPS(15.0f);
    }
    else
    {
        speedBtn.setButtonText("30.00 IPS");
        transport.setSpeedIPS(30.0f);
    }
    repaint();
}

void Deck3Tape::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header (20px)
    auto headerArea = area.removeFromTop(20);
    speedBtn.setBounds(headerArea.removeFromRight(62).reduced(0, 1));
    headerArea.removeFromRight(3);
    bypassBtn.setBounds(headerArea.removeFromRight(48).reduced(0, 1));
    headerArea.removeFromRight(3);
    swapBtn.setBounds(headerArea.removeFromRight(22).reduced(0, 1));
    area.removeFromTop(6);

    // Tape Transport Stage (98px)
    transport.setBounds(area.removeFromTop(98));
    area.removeFromTop(8);

    // Footer (24px)
    area.removeFromBottom(24);

    // 2x2 Knobs grid
    int colW = area.getWidth() / 2;
    int rowH = area.getHeight() / 2;

    satKnob.setBounds(area.getX(), area.getY(), colW, rowH);
    biasCalKnob.setBounds(area.getX() + colW, area.getY(), colW, rowH);
    flutterKnob.setBounds(area.getX(), area.getY() + rowH, colW, rowH);
    wearKnob.setBounds(area.getX() + colW, area.getY() + rowH, colW, rowH);
}

void Deck3Tape::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Drop shadow under deck card
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 4.0f);

    // 2. Deck Tape Gradient Surface
    juce::ColourGradient bg(MauveTheme::deckTape1, 0.0f, 0.0f,
                            MauveTheme::deckTape2, 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 4.0f);

    // 3. Deep Inset Vignette
    juce::ColourGradient vignette(juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getCentreY(),
                                  juce::Colour::fromRGB(8, 4, 12).withAlpha(0.70f), bounds.getRight(), bounds.getBottom(), true);
    g.setGradientFill(vignette);
    g.fillRoundedRectangle(bounds, 4.0f);

    // 4. Top edge highlight line
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.08f));
    g.drawLine(bounds.getX() + 3.0f, bounds.getY() + 1.0f, bounds.getRight() - 3.0f, bounds.getY() + 1.0f, 1.0f);

    // 5. Deck Border
    g.setColour(MauveTheme::deckTapeBrd);
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
    juce::String prefix = "0" + juce::String(slotNumber) + juce::String::fromUTF8(" \xc2\xb7 TAPE REEL");
    g.drawText(prefix, headerArea.withTrimmedLeft(12.0f), juce::Justification::centredLeft, false);

    // Header separator line
    g.setColour(juce::Colour::fromRGB(71, 47, 92));
    g.drawLine(bounds.getX() + 8.0f, headerArea.getBottom() + 3.0f, bounds.getRight() - 8.0f, headerArea.getBottom() + 3.0f, 1.0f);

    // Footer separator line
    float footY = bounds.getBottom() - 32.0f;
    g.drawLine(bounds.getX() + 8.0f, footY, bounds.getRight() - 8.0f, footY, 1.0f);

    // Dynamic Footer Head Bump Label & Value based on 15 vs 30 IPS
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));

    if (currentSpeed == 0) // 15 IPS
    {
        g.drawText("HEAD BUMP: 38 Hz", juce::Rectangle<float>(bounds.getX() + 8.0f, footY + 4.0f, 120.0f, 22.0f), juce::Justification::centredLeft, false);
        g.setColour(MauveTheme::amberBright);
        g.drawText("+2.2 dB", juce::Rectangle<float>(bounds.getRight() - 60.0f, footY + 4.0f, 52.0f, 22.0f), juce::Justification::centredRight, false);
    }
    else // 30 IPS
    {
        g.drawText("HEAD BUMP: 55 Hz", juce::Rectangle<float>(bounds.getX() + 8.0f, footY + 4.0f, 120.0f, 22.0f), juce::Justification::centredLeft, false);
        g.setColour(MauveTheme::amberBright);
        g.drawText("+1.4 dB", juce::Rectangle<float>(bounds.getRight() - 60.0f, footY + 4.0f, 52.0f, 22.0f), juce::Justification::centredRight, false);
    }
}
