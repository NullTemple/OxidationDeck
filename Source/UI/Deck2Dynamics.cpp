#include "Deck2Dynamics.h"

Deck2Dynamics::Deck2Dynamics(juce::AudioProcessorValueTreeState& apvts)
    : threshKnob("THRESHOLD", true),
      ratioKnob("RATIO"),
      attackKnob("ATTACK"),
      releaseKnob("RELEASE"),
      apvtsRef(apvts)
{
    // Mode toggles
    vcaBtn.setButtonText("VCA");
    vcaBtn.setRadioGroupId(101);
    vcaBtn.setClickingTogglesState(true);
    vcaBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(vcaBtn);

    variMuBtn.setButtonText("VARI-MU");
    variMuBtn.setRadioGroupId(101);
    variMuBtn.setClickingTogglesState(true);
    addAndMakeVisible(variMuBtn);

    vcaBtn.onClick = [this]()
    {
        if (auto* param = apvtsRef.getParameter(IDs::compMode.getParamID()))
            param->setValueNotifyingHost(0.0f);
    };
    variMuBtn.onClick = [this]()
    {
        if (auto* param = apvtsRef.getParameter(IDs::compMode.getParamID()))
            param->setValueNotifyingHost(1.0f);
    };

    addAndMakeVisible(vuMeter);

    // Knobs setup
    threshKnob.getSlider().setRange(-30.0, 10.0, 0.1);
    threshKnob.setValueFormat([](double v) { return juce::String((int)v) + " dB"; });
    addAndMakeVisible(threshKnob);

    ratioKnob.getSlider().setRange(0.0, 3.0, 1.0);
    ratioKnob.setValueFormat([](double v) {
        const char* r[] = { "1.5:1", "2:1", "4:1", "10:1" };
        int idx = juce::jlimit(0, 3, (int)std::round(v));
        return r[idx];
    });
    ratioKnob.getSlider().setValue(1.0);
    addAndMakeVisible(ratioKnob);

    attackKnob.getSlider().setRange(0.0, 5.0, 1.0);
    attackKnob.setValueFormat([](double v) {
        const char* a[] = { "0.1 ms", "0.3 ms", "1.0 ms", "3.0 ms", "10 ms", "30 ms" };
        int idx = juce::jlimit(0, 5, (int)std::round(v));
        return a[idx];
    });
    attackKnob.getSlider().setValue(4.0);
    addAndMakeVisible(attackKnob);

    releaseKnob.getSlider().setRange(0.0, 4.0, 1.0);
    releaseKnob.setValueFormat([](double v) {
        const char* rel[] = { "100 ms", "300 ms", "600 ms", "1.2 s", "AUTO" };
        int idx = juce::jlimit(0, 4, (int)std::round(v));
        return rel[idx];
    });
    releaseKnob.getSlider().setValue(4.0);
    addAndMakeVisible(releaseKnob);

    // Interactive Sidechain HPF selector button in footer
    scHpfBtn.setButtonText("SC-HPF: 85 Hz");
    scHpfBtn.setTooltip("Click to cycle Sidechain High-Pass Filter frequency (30 / 60 / 85 / 120 / 160 / 220 Hz)");
    scHpfBtn.onClick = [this]()
    {
        if (auto* p = apvtsRef.getParameter(IDs::compScHpf.getParamID()))
        {
            const float steps[] = { 30.0f, 60.0f, 85.0f, 120.0f, 160.0f, 220.0f };
            float curHz = apvtsRef.getRawParameterValue(IDs::compScHpf.getParamID())->load();
            int nextIdx = 0;
            for (int i = 0; i < 6; ++i)
            {
                if (curHz < steps[i] - 5.0f)
                {
                    nextIdx = i;
                    break;
                }
                nextIdx = (i + 1) % 6;
            }
            p->setValueNotifyingHost(p->convertTo0to1(steps[nextIdx]));
        }
    };
    addAndMakeVisible(scHpfBtn);

    // Sidechain listen button
    listenBtn.setButtonText("LISTEN");
    listenBtn.setClickingTogglesState(true);
    addAndMakeVisible(listenBtn);

    // Swap Deck Order Button
    swapBtn.setButtonText(juce::String::fromUTF8("\xe2\x87\x84"));
    swapBtn.setTooltip("Swap Dynamics position in Signal Chain");
    swapBtn.onClick = [this]() { if (onSwapRight) onSwapRight(); };
    addAndMakeVisible(swapBtn);

    // Bypass Button in Header
    bypassBtn.setButtonText("BYPASS");
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setTooltip("Bypass Master Bus Dynamics");
    addAndMakeVisible(bypassBtn);

    // Attachments
    threshAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::compThresh.getParamID(), threshKnob.getSlider());
    ratioAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::compRatio.getParamID(), ratioKnob.getSlider());
    attackAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::compAttack.getParamID(), attackKnob.getSlider());
    releaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, IDs::compRelease.getParamID(), releaseKnob.getSlider());
    listenAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::compScListen.getParamID(), listenBtn);
    bypassAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, IDs::deck2Bypass.getParamID(), bypassBtn);

    threshKnob.getSlider().setDoubleClickReturnValue(true, -14.0);
    ratioKnob.getSlider().setDoubleClickReturnValue(true, 1.0);
    attackKnob.getSlider().setDoubleClickReturnValue(true, 4.0);
    releaseKnob.getSlider().setDoubleClickReturnValue(true, 4.0);

    apvtsRef.addParameterListener(IDs::compMode.getParamID(), this);
    apvtsRef.addParameterListener(IDs::compScHpf.getParamID(), this);
    syncModeAndHpf();
}

Deck2Dynamics::~Deck2Dynamics()
{
    apvtsRef.removeParameterListener(IDs::compMode.getParamID(), this);
    apvtsRef.removeParameterListener(IDs::compScHpf.getParamID(), this);
}

void Deck2Dynamics::parameterChanged(const juce::String&, float)
{
    juce::Component::SafePointer<Deck2Dynamics> safeThis(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        if (safeThis != nullptr)
            safeThis->syncModeAndHpf();
    });
}

void Deck2Dynamics::syncModeAndHpf()
{
    if (auto* param = apvtsRef.getRawParameterValue(IDs::compMode.getParamID()))
    {
        bool isVariMu = param->load() > 0.5f;
        variMuBtn.setToggleState(isVariMu, juce::dontSendNotification);
        vcaBtn.setToggleState(!isVariMu, juce::dontSendNotification);
    }
    if (auto* hpfP = apvtsRef.getRawParameterValue(IDs::compScHpf.getParamID()))
    {
        int hz = juce::roundToInt(hpfP->load());
        scHpfBtn.setButtonText("SC-HPF: " + juce::String(hz) + " Hz");
    }
    repaint();
}

void Deck2Dynamics::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header (20px)
    auto headerArea = area.removeFromTop(20);
    bypassBtn.setBounds(headerArea.removeFromRight(48).reduced(0, 1));
    headerArea.removeFromRight(3);
    swapBtn.setBounds(headerArea.removeFromRight(22).reduced(0, 1));
    headerArea.removeFromRight(3);
    auto modeArea = headerArea.removeFromRight(82);
    vcaBtn.setBounds(modeArea.removeFromLeft(35).withSizeKeepingCentre(34, 18));
    variMuBtn.setBounds(modeArea.removeFromRight(44).withSizeKeepingCentre(43, 18));

    area.removeFromTop(6);

    // VU Meter Housing (66px)
    vuMeter.setBounds(area.removeFromTop(66));
    area.removeFromTop(8);

    // Footer at bottom (24px)
    auto footerArea = area.removeFromBottom(24);
    listenBtn.setBounds(footerArea.removeFromRight(50).withSizeKeepingCentre(46, 20));
    scHpfBtn.setBounds(footerArea.removeFromLeft(105).withSizeKeepingCentre(102, 20));

    // 2x2 Knobs grid
    int colW = area.getWidth() / 2;
    int rowH = area.getHeight() / 2;

    threshKnob.setBounds(area.getX(), area.getY(), colW, rowH);
    ratioKnob.setBounds(area.getX() + colW, area.getY(), colW, rowH);
    attackKnob.setBounds(area.getX(), area.getY() + rowH, colW, rowH);
    releaseKnob.setBounds(area.getX() + colW, area.getY() + rowH, colW, rowH);
}

void Deck2Dynamics::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Drop shadow under deck card
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 4.0f);

    // 2. Deck Comp Gradient Surface
    juce::ColourGradient bg(MauveTheme::deckComp1, 0.0f, 0.0f,
                            MauveTheme::deckComp2, 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 4.0f);

    // 3. Top edge highlight line
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.06f));
    g.drawLine(bounds.getX() + 3.0f, bounds.getY() + 1.0f, bounds.getRight() - 3.0f, bounds.getY() + 1.0f, 1.0f);

    // 4. Deck Border
    g.setColour(MauveTheme::deckCompBrd);
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

    // Deck Title with Dynamic Slot Number
    g.setColour(isBypassed ? MauveTheme::silkscreen : MauveTheme::textBright);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    juce::String prefix = "0" + juce::String(slotNumber) + juce::String::fromUTF8(" \xc2\xb7 DYNAMICS");
    g.drawText(prefix, headerArea.withTrimmedLeft(12.0f), juce::Justification::centredLeft, false);

    // Header separator line
    g.setColour(juce::Colour::fromRGB(52, 34, 71));
    g.drawLine(bounds.getX() + 8.0f, headerArea.getBottom() + 3.0f, bounds.getRight() - 8.0f, headerArea.getBottom() + 3.0f, 1.0f);

    // Footer separator line
    float footY = bounds.getBottom() - 32.0f;
    g.drawLine(bounds.getX() + 8.0f, footY, bounds.getRight() - 8.0f, footY, 1.0f);
}
