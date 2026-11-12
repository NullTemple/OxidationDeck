#include "Deck4Limiter.h"

Deck4Limiter::Deck4Limiter(juce::AudioProcessorValueTreeState& apvts)
    : ceilingKnob("CEILING", true),
      inGainKnob("IN GAIN"),
      lookaheadKnob("LOOKAHEAD"),
      releaseKnob("RELEASE")
{
    addAndMakeVisible(radar);

    // Knobs setup
    ceilingKnob.getSlider().setRange(-12.0, 0.0, 0.05);
    ceilingKnob.setValueFormat([](double v) { return juce::String(v, 1) + " dBTP"; });
    addAndMakeVisible(ceilingKnob);

    inGainKnob.getSlider().setRange(-12.0, 12.0, 0.1);
    inGainKnob.setValueFormat([](double v) {
        if (std::abs(v) < 0.05)
            return juce::String("0.0 dB");
        return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " dB";
    });
    addAndMakeVisible(inGainKnob);

    lookaheadKnob.getSlider().setRange(0.5, 5.0, 0.1);
    lookaheadKnob.setValueFormat([](double v) { return juce::String(v, 1) + " ms"; });
    addAndMakeVisible(lookaheadKnob);

    releaseKnob.getSlider().setRange(0.0, 3.0, 1.0);
    releaseKnob.setValueFormat([](double v) {
        const char* rel[] = { "FAST", "MEDIUM", "SLOW", "ADAPTIVE" };
        int idx = juce::jlimit(0, 3, static_cast<int>(std::round(v)));
        return rel[idx];
    });
    addAndMakeVisible(releaseKnob);

    // Attachments
    ceilingAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::limCeiling.getParamID(), ceilingKnob.getSlider());
    inGainAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::limInGain.getParamID(), inGainKnob.getSlider());
    lookaheadAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::limLookahead.getParamID(), lookaheadKnob.getSlider());
    releaseAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::limRelease.getParamID(), releaseKnob.getSlider());

    ceilingKnob.getSlider().setDoubleClickReturnValue(true, -1.0);
    inGainKnob.getSlider().setDoubleClickReturnValue(true, 0.0);
    lookaheadKnob.getSlider().setDoubleClickReturnValue(true, 2.0);
    releaseKnob.getSlider().setDoubleClickReturnValue(true, 3.0);

    // Bypass Button in Header
    bypassBtn.setButtonText("BYPASS");
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setTooltip("Bypass Deck 4 (True-Peak Limiter)");
    addAndMakeVisible(bypassBtn);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::deck4Bypass.getParamID(), bypassBtn);
}

void Deck4Limiter::mouseDown(const juce::MouseEvent& event)
{
    if (event.y >= getHeight() - 32 && onResetClips)
    {
        onResetClips();
        currentClips = 0;
        repaint();
    }
}

void Deck4Limiter::updateTelemetry(float grDb, int clips, float lufsInt, float tpMax, float lufsSt, float lra)
{
    radar.setTelemetry(lufsInt, tpMax, lufsSt, lra, grDb);
    if (currentClips != clips)
    {
        currentClips = clips;
        repaint();
    }
}

void Deck4Limiter::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header (20px)
    auto headerArea = area.removeFromTop(20);
    bypassBtn.setBounds(headerArea.removeFromRight(50).reduced(0, 1));
    area.removeFromTop(6);

    // EBU R128 Radar Screen (74px)
    radar.setBounds(area.removeFromTop(74));
    area.removeFromTop(8);

    // Footer (24px)
    area.removeFromBottom(24);

    // 2x2 Knobs grid
    int colW = area.getWidth() / 2;
    int rowH = area.getHeight() / 2;

    ceilingKnob.setBounds(area.getX(), area.getY(), colW, rowH);
    inGainKnob.setBounds(area.getX() + colW, area.getY(), colW, rowH);
    lookaheadKnob.setBounds(area.getX(), area.getY() + rowH, colW, rowH);
    releaseKnob.setBounds(area.getX() + colW, area.getY() + rowH, colW, rowH);
}

void Deck4Limiter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Drop shadow under deck card
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 4.0f);

    // 2. Deck Limiter Gradient Surface
    juce::ColourGradient bg(MauveTheme::deckLimiter1, 0.0f, 0.0f,
                            MauveTheme::deckLimiter2, 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 4.0f);

    // 3. Top edge highlight line
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.05f));
    g.drawLine(bounds.getX() + 3.0f, bounds.getY() + 1.0f, bounds.getRight() - 3.0f, bounds.getY() + 1.0f, 1.0f);

    // 4. Deck Border
    g.setColour(MauveTheme::deckLimiterBrd);
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
    g.setColour(isBypassed ? juce::Colour::fromRGB(55, 36, 74) : MauveTheme::amberBright);
    g.fillEllipse(headerArea.getX(), headerArea.getCentreY() - 3.5f, 7.0f, 7.0f);

    // Deck Title
    g.setColour(isBypassed ? MauveTheme::silkscreen : MauveTheme::textBright);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText(juce::String::fromUTF8("04 \xc2\xb7 LIMITER"), headerArea.withTrimmedLeft(12.0f), juce::Justification::centredLeft, false);

    // Header separator line
    g.setColour(juce::Colour::fromRGB(46, 29, 62));
    g.drawLine(bounds.getX() + 8.0f, headerArea.getBottom() + 3.0f, bounds.getRight() - 8.0f, headerArea.getBottom() + 3.0f, 1.0f);

    // Footer separator line
    float footY = bounds.getBottom() - 32.0f;
    g.drawLine(bounds.getX() + 8.0f, footY, bounds.getRight() - 8.0f, footY, 1.0f);

    // Footer label & value
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText("ISP SAFE", juce::Rectangle<float>(bounds.getX() + 8.0f, footY + 4.0f, 70.0f, 22.0f), juce::Justification::centredLeft, false);

    g.setColour(currentClips > 0 ? MauveTheme::amberBright : MauveTheme::amber);
    g.drawText(juce::String(currentClips) + " CLIPS", juce::Rectangle<float>(bounds.getRight() - 65.0f, footY + 4.0f, 57.0f, 22.0f), juce::Justification::centredRight, false);
}
