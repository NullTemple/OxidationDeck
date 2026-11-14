#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    static constexpr int kOrderTable[6][3] = {
        { 0, 1, 2 }, // 0: PRE  > COMP > TAPE
        { 0, 2, 1 }, // 1: PRE  > TAPE > COMP
        { 1, 0, 2 }, // 2: COMP > PRE  > TAPE
        { 1, 2, 0 }, // 3: COMP > TAPE > PRE
        { 2, 0, 1 }, // 4: TAPE > PRE  > COMP
        { 2, 1, 0 }  // 5: TAPE > COMP > PRE
    };
}

OxidationDeckAudioProcessorEditor::OxidationDeckAudioProcessorEditor(OxidationDeckAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      deck1(p.getAPVTS()),
      deck2(p.getAPVTS()),
      deck3(p.getAPVTS()),
      deck4(p.getAPVTS())
{
    setLookAndFeel(&lnf);
    addAndMakeVisible(contentPanel);

    // Signal Chain Order Selector (6 Permutations of Decks 1, 2, 3)
    chainBox.addItemList(juce::StringArray {
        juce::String::fromUTF8("PRE \xe2\x96\xb8 COMP \xe2\x96\xb8 TAPE"),
        juce::String::fromUTF8("PRE \xe2\x96\xb8 TAPE \xe2\x96\xb8 COMP"),
        juce::String::fromUTF8("COMP \xe2\x96\xb8 PRE \xe2\x96\xb8 TAPE"),
        juce::String::fromUTF8("COMP \xe2\x96\xb8 TAPE \xe2\x96\xb8 PRE"),
        juce::String::fromUTF8("TAPE \xe2\x96\xb8 PRE \xe2\x96\xb8 COMP"),
        juce::String::fromUTF8("TAPE \xe2\x96\xb8 COMP \xe2\x96\xb8 PRE")
    }, 1);
    chainBox.setTooltip("Reorder Decks 1-3 in the Master Bus Signal Chain");
    contentPanel.addAndMakeVisible(chainBox);
    chainAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), IDs::chainOrder.getParamID(), chainBox);

    // Preset selector
    presetBox.addItemList(juce::StringArray {
        "01: MASTER // 30 IPS TUBE GLUE",
        "02: MASTER // VCA SNAP & IRON",
        "03: MASTER // GERMANIUM VELVET",
        "04: STEM // DRUM REEL PUNCH",
        "05: EXPERIMENTAL // OXIDE SHED"
    }, 1);
    presetBox.setSelectedId(processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]() { applyFactoryPreset(presetBox.getSelectedId()); };
    contentPanel.addAndMakeVisible(presetBox);

    // Oversampling buttons
    os2xBtn.setButtonText("2x");
    os4xBtn.setButtonText("4x");
    os8xBtn.setButtonText("8x");
    os2xBtn.setRadioGroupId(201);
    os4xBtn.setRadioGroupId(201);
    os8xBtn.setRadioGroupId(201);
    os2xBtn.setClickingTogglesState(true);
    os4xBtn.setClickingTogglesState(true);
    os8xBtn.setClickingTogglesState(true);
    os4xBtn.setToggleState(true, juce::dontSendNotification);

    contentPanel.addAndMakeVisible(os2xBtn);
    contentPanel.addAndMakeVisible(os4xBtn);
    contentPanel.addAndMakeVisible(os8xBtn);

    os2xBtn.onClick = [this]() {
        if (auto* param = processorRef.getAPVTS().getParameter(IDs::oversample.getParamID()))
            param->setValueNotifyingHost(0.0f);
    };
    os4xBtn.onClick = [this]() {
        if (auto* param = processorRef.getAPVTS().getParameter(IDs::oversample.getParamID()))
            param->setValueNotifyingHost(0.5f);
    };
    os8xBtn.onClick = [this]() {
        if (auto* param = processorRef.getAPVTS().getParameter(IDs::oversample.getParamID()))
            param->setValueNotifyingHost(1.0f);
    };

    // Routing Mode buttons
    stereoBtn.setButtonText("STEREO");
    msBtn.setButtonText("M / S");
    stereoBtn.setRadioGroupId(202);
    msBtn.setRadioGroupId(202);
    stereoBtn.setClickingTogglesState(true);
    msBtn.setClickingTogglesState(true);
    stereoBtn.setToggleState(true, juce::dontSendNotification);

    contentPanel.addAndMakeVisible(stereoBtn);
    contentPanel.addAndMakeVisible(msBtn);

    stereoBtn.onClick = [this]() {
        if (auto* param = processorRef.getAPVTS().getParameter(IDs::routingMode.getParamID()))
            param->setValueNotifyingHost(0.0f);
    };
    msBtn.onClick = [this]() {
        if (auto* param = processorRef.getAPVTS().getParameter(IDs::routingMode.getParamID()))
            param->setValueNotifyingHost(1.0f);
    };

    masterBypassBtn.setButtonText("BYPASS");
    masterBypassBtn.setClickingTogglesState(true);
    masterBypassBtn.setTooltip("Master Bypass (Latency-Compensated A/B Compare)");
    contentPanel.addAndMakeVisible(masterBypassBtn);
    masterBypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), IDs::bypass.getParamID(), masterBypassBtn);

    scaleBox.addItem("75%", 1);
    scaleBox.addItem("100%", 2);
    scaleBox.addItem("125%", 3);
    scaleBox.setSelectedId(2, juce::dontSendNotification);
    scaleBox.setJustificationType(juce::Justification::centred);
    scaleBox.setTooltip("UI Scale Factor");
    scaleBox.onChange = [this]() {
        switch (scaleBox.getSelectedId())
        {
            case 1: applyUiScale(0.75f); break;
            case 2: applyUiScale(1.0f);  break;
            case 3: applyUiScale(1.25f); break;
            default: break;
        }
    };
    contentPanel.addAndMakeVisible(scaleBox);

    // Decks & Swap Callbacks
    contentPanel.addAndMakeVisible(deck1);
    contentPanel.addAndMakeVisible(deck2);
    contentPanel.addAndMakeVisible(deck3);
    contentPanel.addAndMakeVisible(deck4);

    auto swapDeckByType = [this](int deckType)
    {
        auto* p = processorRef.getAPVTS().getRawParameterValue(IDs::chainOrder.getParamID());
        int curOrder = p ? juce::jlimit(0, 5, juce::roundToInt(p->load())) : 0;
        for (int slot = 0; slot < 3; ++slot)
        {
            if (kOrderTable[curOrder][slot] == deckType)
            {
                swapDeckAtSlot(slot);
                break;
            }
        }
    };

    deck1.onSwapRight = [swapDeckByType]() { swapDeckByType(0); };
    deck2.onSwapRight = [swapDeckByType]() { swapDeckByType(1); };
    deck3.onSwapRight = [swapDeckByType]() { swapDeckByType(2); };
    deck4.onResetClips = [this]() { processorRef.resetLimiterClips(); };

    processorRef.getAPVTS().addParameterListener(IDs::bypass.getParamID(), this);
    processorRef.getAPVTS().addParameterListener(IDs::chainOrder.getParamID(), this);
    processorRef.getAPVTS().addParameterListener(IDs::oversample.getParamID(), this);
    processorRef.getAPVTS().addParameterListener(IDs::routingMode.getParamID(), this);

    setSize(kBaseWidth, kBaseHeight);
    syncHeaderControls();
    startTimerHz(30);
}

OxidationDeckAudioProcessorEditor::~OxidationDeckAudioProcessorEditor()
{
    stopTimer();
    processorRef.getAPVTS().removeParameterListener(IDs::bypass.getParamID(), this);
    processorRef.getAPVTS().removeParameterListener(IDs::chainOrder.getParamID(), this);
    processorRef.getAPVTS().removeParameterListener(IDs::oversample.getParamID(), this);
    processorRef.getAPVTS().removeParameterListener(IDs::routingMode.getParamID(), this);
    setLookAndFeel(nullptr);
}

void OxidationDeckAudioProcessorEditor::mouseDownContent(const juce::MouseEvent& event)
{
    if (event.position.x >= 24.0f && event.position.x <= 160.0f
        && event.position.y >= 6.0f && event.position.y <= 46.0f)
    {
        if (auto* p = processorRef.getAPVTS().getParameter(IDs::bypass.getParamID()))
        {
            const float nextVal = (p->getValue() > 0.5f) ? 0.0f : 1.0f;
            p->beginChangeGesture();
            p->setValueNotifyingHost(nextVal);
            p->endChangeGesture();
        }
    }
}

void OxidationDeckAudioProcessorEditor::parameterChanged(const juce::String&, float)
{
    juce::Component::SafePointer<OxidationDeckAudioProcessorEditor> safeThis(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        if (safeThis != nullptr)
        {
            safeThis->syncHeaderControls();
            safeThis->resizedContent();
            safeThis->contentPanel.repaint();
        }
    });
}

void OxidationDeckAudioProcessorEditor::syncHeaderControls()
{
    int expectedPresetId = processorRef.getCurrentProgram() + 1;
    if (!presetBox.isPopupActive() && presetBox.getSelectedId() != expectedPresetId)
        presetBox.setSelectedId(expectedPresetId, juce::dontSendNotification);

    if (auto* osP = processorRef.getAPVTS().getRawParameterValue(IDs::oversample.getParamID()))
    {
        int osIdx = juce::jlimit(0, 2, juce::roundToInt(osP->load()));
        os2xBtn.setToggleState(osIdx == 0, juce::dontSendNotification);
        os4xBtn.setToggleState(osIdx == 1, juce::dontSendNotification);
        os8xBtn.setToggleState(osIdx == 2, juce::dontSendNotification);
    }
    if (auto* rmP = processorRef.getAPVTS().getRawParameterValue(IDs::routingMode.getParamID()))
    {
        bool isMs = rmP->load() > 0.5f;
        msBtn.setToggleState(isMs, juce::dontSendNotification);
        stereoBtn.setToggleState(!isMs, juce::dontSendNotification);
    }
}

void OxidationDeckAudioProcessorEditor::swapDeckAtSlot(int slotIndex)
{
    auto* p = processorRef.getAPVTS().getRawParameterValue(IDs::chainOrder.getParamID());
    int curOrder = p ? juce::jlimit(0, 5, juce::roundToInt(p->load())) : 0;

    int slots[3] = {
        kOrderTable[curOrder][0],
        kOrderTable[curOrder][1],
        kOrderTable[curOrder][2]
    };

    // Swap slotIndex with next slot (0 <-> 1, 1 <-> 2, 2 <-> 0)
    int nextSlot = (slotIndex + 1) % 3;
    std::swap(slots[slotIndex], slots[nextSlot]);

    // Find matching permutation index
    for (int idx = 0; idx < 6; ++idx)
    {
        if (kOrderTable[idx][0] == slots[0] &&
            kOrderTable[idx][1] == slots[1] &&
            kOrderTable[idx][2] == slots[2])
        {
            if (auto* param = processorRef.getAPVTS().getParameter(IDs::chainOrder.getParamID()))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(idx)));
            break;
        }
    }
}

void OxidationDeckAudioProcessorEditor::applyFactoryPreset(int presetId)
{
    processorRef.applyFactoryPreset(presetId - 1);
    syncHeaderControls();
    resizedContent();
    contentPanel.repaint();
}

void OxidationDeckAudioProcessorEditor::applyUiScale(float newScale)
{
    uiScale = newScale;
    setSize(static_cast<int>(std::round(static_cast<float>(kBaseWidth) * uiScale)),
            static_cast<int>(std::round(static_cast<float>(kBaseHeight) * uiScale)));
    resized();
}

void OxidationDeckAudioProcessorEditor::timerCallback()
{
    int expectedPresetId = processorRef.getCurrentProgram() + 1;
    if (!presetBox.isPopupActive() && presetBox.getSelectedId() != expectedPresetId)
        presetBox.setSelectedId(expectedPresetId, juce::dontSendNotification);

    float targetInL = processorRef.getInputPeakL();
    float targetInR = processorRef.getInputPeakR();
    float targetOutL = processorRef.getOutputPeakL();
    float targetOutR = processorRef.getOutputPeakR();
    float targetCorr = processorRef.getCorrelation();

    // Fast attack (instant peak catch), smooth decay
    smoothInL = (targetInL > smoothInL) ? targetInL : smoothInL * 0.82f + targetInL * 0.18f;
    smoothInR = (targetInR > smoothInR) ? targetInR : smoothInR * 0.82f + targetInR * 0.18f;
    smoothOutL = (targetOutL > smoothOutL) ? targetOutL : smoothOutL * 0.82f + targetOutL * 0.18f;
    smoothOutR = (targetOutR > smoothOutR) ? targetOutR : smoothOutR * 0.82f + targetOutR * 0.18f;
    peakHoldInL  = (smoothInL > peakHoldInL)   ? smoothInL  : peakHoldInL * 0.965f;
    peakHoldInR  = (smoothInR > peakHoldInR)   ? smoothInR  : peakHoldInR * 0.965f;
    peakHoldOutL = (smoothOutL > peakHoldOutL) ? smoothOutL : peakHoldOutL * 0.965f;
    peakHoldOutR = (smoothOutR > peakHoldOutR) ? smoothOutR : peakHoldOutR * 0.965f;
    smoothCorr = smoothCorr * 0.80f + targetCorr * 0.20f;

    deck1.updateHarmonicTelemetry(processorRef.getPreampHarmonicActivity());
    deck2.updateVUMeter(processorRef.getGainReductionL(), processorRef.getGainReductionR());
    deck3.updateFluxTelemetry(processorRef.getTapeFluxIntensity());
    deck4.updateTelemetry(processorRef.getLimiterGainReduction(),
                          processorRef.getLimiterClipCount(),
                          processorRef.getLimiterIntegratedLufs(),
                          processorRef.getLimiterTruePeak(),
                          processorRef.getLimiterShortTermLufs(),
                          processorRef.getLimiterLra());

    contentPanel.repaint();
}

void OxidationDeckAudioProcessorEditor::resized()
{
    contentPanel.setTransform(juce::AffineTransform());
    contentPanel.setBounds(0, 0, kBaseWidth, kBaseHeight);
    contentPanel.setTransform(juce::AffineTransform::scale(uiScale));
    resizedContent();
}

void OxidationDeckAudioProcessorEditor::resizedContent()
{
    juce::Rectangle<int> area(0, 0, kBaseWidth, kBaseHeight);

    // 19-inch chassis inner area (leave 20px left and right for rack ears)
    auto innerArea = area.reduced(20, 6);

    // Header bar (40px)
    auto headerArea = innerArea.removeFromTop(40);
    headerArea.removeFromLeft(295); // brand logo & model badge space

    // Header controls aligned right
    auto rightHeader = headerArea.removeFromRight(875);

    auto scaleArea = rightHeader.removeFromRight(62).reduced(0, 7);
    scaleBox.setBounds(scaleArea);
    rightHeader.removeFromRight(8);

    auto bypassArea = rightHeader.removeFromRight(60).reduced(0, 8);
    masterBypassBtn.setBounds(bypassArea);
    rightHeader.removeFromRight(8);

    auto msArea = rightHeader.removeFromRight(106).reduced(0, 8);
    stereoBtn.setBounds(msArea.removeFromLeft(53));
    msBtn.setBounds(msArea);
    rightHeader.removeFromRight(10);

    auto osArea = rightHeader.removeFromRight(90).reduced(0, 8);
    int osBtnW = osArea.getWidth() / 3;
    os2xBtn.setBounds(osArea.removeFromLeft(osBtnW));
    os4xBtn.setBounds(osArea.removeFromLeft(osBtnW));
    os8xBtn.setBounds(osArea);
    rightHeader.removeFromRight(10);

    presetBox.setBounds(rightHeader.removeFromRight(222).reduced(0, 7));
    rightHeader.removeFromRight(60); // space for PRESET label

    chainBox.setBounds(rightHeader.removeFromRight(186).reduced(0, 7));

    // Footer status bar (36px)
    innerArea.removeFromBottom(36);

    // Center deck bays (4-column grid, 6px gap)
    auto decksArea = innerArea.reduced(5, 5);
    int deckW = (decksArea.getWidth() - 18) / 4;
    int deckH = decksArea.getHeight();

    // Dynamically position Decks 1, 2, 3 left-to-right according to Signal Chain Order!
    auto* p = processorRef.getAPVTS().getRawParameterValue(IDs::chainOrder.getParamID());
    int curOrder = p ? juce::jlimit(0, 5, juce::roundToInt(p->load())) : 0;

    juce::Component* deckPtrs[3] = { &deck1, &deck2, &deck3 };
    for (int slot = 0; slot < 3; ++slot)
    {
        int deckType = kOrderTable[curOrder][slot];
        int xPos = decksArea.getX() + (deckW + 6) * slot;
        deckPtrs[deckType]->setBounds(xPos, decksArea.getY(), deckW, deckH);

        if (deckType == 0)      deck1.setSlotNumber(slot + 1);
        else if (deckType == 1) deck2.setSlotNumber(slot + 1);
        else if (deckType == 2) deck3.setSlotNumber(slot + 1);
    }

    // Deck 4 is always the final brickwall True-Peak Limiter in Slot 4
    deck4.setBounds(decksArea.getX() + (deckW + 6) * 3, decksArea.getY(), deckW, deckH);
}

void OxidationDeckAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(MauveTheme::spaceBg);
}

void OxidationDeckAudioProcessorEditor::paintContent(juce::Graphics& g)
{
    // Outer Space
    g.fillAll(MauveTheme::spaceBg);

    juce::Rectangle<float> totalBounds(0.0f, 0.0f, static_cast<float>(kBaseWidth), static_cast<float>(kBaseHeight));

    // Rack Ears Screws (Left 3, Right 3, diameter 12px)
    float screwDiameter = 12.0f;
    float leftX = 10.0f;
    float rightX = totalBounds.getRight() - 10.0f;

    float y1 = 24.0f;
    float y2 = totalBounds.getCentreY();
    float y3 = totalBounds.getBottom() - 24.0f;

    MauveLookAndFeel::drawChassisScrew(g, leftX, y1, screwDiameter);
    MauveLookAndFeel::drawChassisScrew(g, leftX, y2, screwDiameter);
    MauveLookAndFeel::drawChassisScrew(g, leftX, y3, screwDiameter);

    MauveLookAndFeel::drawChassisScrew(g, rightX, y1, screwDiameter);
    MauveLookAndFeel::drawChassisScrew(g, rightX, y2, screwDiameter);
    MauveLookAndFeel::drawChassisScrew(g, rightX, y3, screwDiameter);

    // 1. Chassis Outer Bevels
    auto innerBounds = totalBounds.reduced(20.0f, 6.0f);

    // Deep outer chassis shadow
    g.setColour(juce::Colours::black.withAlpha(0.95f));
    g.fillRoundedRectangle(innerBounds.translated(0.0f, 3.0f), 5.0f);

    // Chassis gradient
    juce::ColourGradient chassisGrad(MauveTheme::chassisOuter1, 0.0f, innerBounds.getY(),
                                     MauveTheme::chassisOuter2, 0.0f, innerBounds.getBottom(), false);
    g.setGradientFill(chassisGrad);
    g.fillRoundedRectangle(innerBounds, 5.0f);

    // Chassis border
    g.setColour(MauveTheme::chassisBorder);
    g.drawRoundedRectangle(innerBounds, 5.0f, 1.5f);

    // Inset top-left 1px highlight line
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.12f));
    g.drawLine(innerBounds.getX() + 4.0f, innerBounds.getY() + 1.0f, innerBounds.getRight() - 4.0f, innerBounds.getY() + 1.0f, 1.0f);
    g.drawLine(innerBounds.getX() + 1.0f, innerBounds.getY() + 4.0f, innerBounds.getX() + 1.0f, innerBounds.getBottom() - 4.0f, 1.0f);

    // Inset bottom-right 1px shadow line
    g.setColour(juce::Colours::black.withAlpha(0.95f));
    g.drawLine(innerBounds.getX() + 4.0f, innerBounds.getBottom() - 1.0f, innerBounds.getRight() - 4.0f, innerBounds.getBottom() - 1.0f, 1.0f);
    g.drawLine(innerBounds.getRight() - 1.0f, innerBounds.getY() + 4.0f, innerBounds.getRight() - 1.0f, innerBounds.getBottom() - 4.0f, 1.0f);

    // Header & Footer backgrounds
    auto headerRect = innerBounds.removeFromTop(40.0f).toNearestInt();
    drawChassisHeader(g, headerRect);

    auto footerRect = innerBounds.removeFromBottom(36.0f).toNearestInt();
    drawChassisFooter(g, footerRect);

    // Glowing Signal-Flow Conduit Chevrons between the 4 Deck Bays
    auto* bpParam = processorRef.getAPVTS().getRawParameterValue(IDs::bypass.getParamID());
    bool isMasterBypassed = (bpParam != nullptr && bpParam->load() > 0.5f);
    auto decksArea = innerBounds.reduced(5.0f, 5.0f);
    float deckW = (decksArea.getWidth() - 18.0f) / 4.0f;
    float midDeckY = decksArea.getCentreY();
    float sigActivity = isMasterBypassed
        ? 0.12f
        : juce::jlimit(0.25f, 1.0f, 0.30f + juce::jmax(smoothInL, smoothOutL) * 1.2f);

    for (int gapIdx = 0; gapIdx < 3; ++gapIdx)
    {
        float gapCenterX = decksArea.getX() + deckW * (gapIdx + 1) + 6.0f * gapIdx + 3.0f;
        g.setColour(MauveTheme::amber.withAlpha(0.45f * sigActivity));
        g.drawVerticalLine(static_cast<int>(gapCenterX), midDeckY - 26.0f, midDeckY + 26.0f);

        // Directional Signal Conduit Chevron Badge
        juce::Path chev;
        chev.startNewSubPath(gapCenterX - 1.8f, midDeckY - 3.2f);
        chev.lineTo(gapCenterX + 2.2f, midDeckY);
        chev.lineTo(gapCenterX - 1.8f, midDeckY + 3.2f);
        g.setColour(MauveTheme::amberBright.withAlpha(sigActivity));
        g.strokePath(chev, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }
}

void OxidationDeckAudioProcessorEditor::drawChassisHeader(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto* bpParam = processorRef.getAPVTS().getRawParameterValue(IDs::bypass.getParamID());
    bool isMasterBypassed = (bpParam != nullptr && bpParam->load() > 0.5f);

    auto fArea = area.toFloat();

    // Background
    g.setColour(MauveTheme::headerBg);
    g.fillRoundedRectangle(fArea.removeFromTop(39.0f), 4.0f);

    g.setColour(MauveTheme::headerBorder);
    g.drawHorizontalLine(area.getBottom() - 1, (float)area.getX(), (float)area.getRight());

    // Inset top highlight
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.08f));
    g.drawHorizontalLine(area.getY() + 1, (float)area.getX() + 4.0f, (float)area.getRight() - 4.0f);

    // Left: Amber Tally LED (dims when Master Bypassed; click LED/logo to toggle)
    float tallyX = (float)area.getX() + 14.0f;
    float tallyY = (float)area.getCentreY();

    if (!isMasterBypassed)
    {
        g.setColour(MauveTheme::amber.withAlpha(0.45f));
        g.fillEllipse(tallyX - 5.5f, tallyY - 5.5f, 11.0f, 11.0f);
    }

    g.setColour(isMasterBypassed ? juce::Colour::fromRGB(65, 42, 85) : MauveTheme::amberBright);
    g.fillEllipse(tallyX - 3.0f, tallyY - 3.0f, 6.0f, 6.0f);

    // Brand Name
    g.setColour(isMasterBypassed ? MauveTheme::silkscreen : MauveTheme::textBright);
    g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    g.drawText("MAUVE ZONE", juce::Rectangle<float>(tallyX + 13.0f, (float)area.getY(), 105.0f, (float)area.getHeight()),
               juce::Justification::centredLeft, false);

    // Model
    g.setColour(isMasterBypassed ? MauveTheme::silkscreen : MauveTheme::amberBright);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText(isMasterBypassed ? "MZOD-1 // BYPASSED" : "MZOD-1 // MASTER BUS",
               juce::Rectangle<float>(tallyX + 114.0f, (float)area.getY(), 165.0f, (float)area.getHeight()),
               juce::Justification::centredLeft, false);

    // CHAIN label
    if (chainBox.getWidth() > 0)
    {
        float cLabelX = (float)chainBox.getX() - 52.0f;
        g.setColour(MauveTheme::silkscreen);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText("CHAIN", juce::Rectangle<float>(cLabelX, (float)area.getY(), 46.0f, (float)area.getHeight()),
                   juce::Justification::centredRight, false);
    }

    // PRESET label
    if (presetBox.getWidth() > 0)
    {
        float pLabelX = (float)presetBox.getX() - 54.0f;
        g.setColour(MauveTheme::silkscreen);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText("PRESET", juce::Rectangle<float>(pLabelX, (float)area.getY(), 48.0f, (float)area.getHeight()),
                   juce::Justification::centredRight, false);
    }
}

void OxidationDeckAudioProcessorEditor::drawChassisFooter(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto fArea = area.toFloat();

    // Background
    g.setColour(MauveTheme::footerBg);
    g.fillRect(fArea);

    g.setColour(MauveTheme::headerBorder);
    g.drawHorizontalLine(area.getY(), (float)area.getX(), (float)area.getRight());

    float midY = (float)area.getCentreY();
    float startX = (float)area.getX() + 14.0f;

    // 1. Stereo Input L/R Meter
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("INPUT L/R", juce::Rectangle<float>(startX, (float)area.getY(), 62.0f, (float)area.getHeight()), juce::Justification::centredLeft, false);
    startX += 66.0f;

    auto toFillRatio = [](float linear)
    {
        if (linear <= 1e-4f) return 0.0f;
        float db = juce::Decibels::gainToDecibels(linear, -60.0f);
        return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 66.0f);
    };

    auto toDbText = [](float linear, bool isTP)
    {
        if (linear <= 1e-3f) return juce::String("-INF");
        float db = juce::Decibels::gainToDecibels(linear, -60.0f);
        return (db > 0.0f ? "+" : "") + juce::String(db, 1) + (isTP ? " dBTP" : " dB");
    };

    auto drawMeter = [&](float x, float y, float w, float fillRatio, float peakHoldRatio)
    {
        g.setColour(juce::Colour::fromRGB(8, 5, 12));
        g.fillRoundedRectangle(x, y, w, 4.5f, 1.0f);
        g.setColour(juce::Colour::fromRGB(52, 34, 71));
        g.drawRoundedRectangle(x, y, w, 4.5f, 1.0f, 0.8f);

        if (fillRatio > 0.01f)
        {
            juce::ColourGradient grad(juce::Colour::fromRGB(92, 59, 130), x, y,
                                      juce::Colour::fromRGB(255, 145, 0), x + w, y, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(x, y, w * fillRatio, 4.5f, 1.0f);
        }

        if (peakHoldRatio > 0.02f)
        {
            float px = x + juce::jlimit(1.0f, w - 1.5f, w * peakHoldRatio);
            g.setColour(MauveTheme::amberBright);
            g.fillRect(px, y, 1.5f, 4.5f);
        }
    };

    float inRatioL = toFillRatio(smoothInL);
    float inRatioR = toFillRatio(smoothInR);
    drawMeter(startX, midY - 5.5f, 72.0f, inRatioL, toFillRatio(peakHoldInL));
    drawMeter(startX, midY + 1.0f, 72.0f, inRatioR, toFillRatio(peakHoldInR));
    startX += 80.0f;

    g.setColour(MauveTheme::amberBright);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    float maxIn = juce::jmax(smoothInL, smoothInR);
    g.drawText(toDbText(maxIn, false), juce::Rectangle<float>(startX, (float)area.getY(), 50.0f, (float)area.getHeight()), juce::Justification::centredLeft, false);
    startX += 56.0f;

    // 2. Stereo Output L/R Meter
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("OUTPUT L/R", juce::Rectangle<float>(startX, (float)area.getY(), 70.0f, (float)area.getHeight()), juce::Justification::centredLeft, false);
    startX += 74.0f;

    float outRatioL = toFillRatio(smoothOutL);
    float outRatioR = toFillRatio(smoothOutR);
    drawMeter(startX, midY - 5.5f, 72.0f, outRatioL, toFillRatio(peakHoldOutL));
    drawMeter(startX, midY + 1.0f, 72.0f, outRatioR, toFillRatio(peakHoldOutR));
    startX += 80.0f;

    g.setColour(MauveTheme::amberBright);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    float maxOut = juce::jmax(smoothOutL, smoothOutR);
    g.drawText(toDbText(maxOut, true), juce::Rectangle<float>(startX, (float)area.getY(), 64.0f, (float)area.getHeight()), juce::Justification::centredLeft, false);
    startX += 70.0f;

    // 3. Phase Correlation Meter
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("CORRELATION", juce::Rectangle<float>(startX, (float)area.getY(), 78.0f, (float)area.getHeight()), juce::Justification::centredLeft, false);
    startX += 82.0f;

    float corrTrackW = 48.0f;
    g.setColour(juce::Colour::fromRGB(8, 5, 12));
    g.fillRoundedRectangle(startX, midY - 2.5f, corrTrackW, 5.0f, 1.0f);
    g.setColour(juce::Colour::fromRGB(52, 34, 71));
    g.drawRoundedRectangle(startX, midY - 2.5f, corrTrackW, 5.0f, 1.0f, 0.8f);

    float centerCorrX = startX + corrTrackW * 0.5f;
    g.setColour(juce::Colour::fromRGB(95, 65, 125));
    g.drawVerticalLine(static_cast<int>(centerCorrX), midY - 2.5f, midY + 2.5f);

    float corrNorm = (juce::jlimit(-1.0f, 1.0f, smoothCorr) + 1.0f) * 0.5f;
    float markerX = startX + (corrTrackW - 3.0f) * corrNorm;
    juce::Colour corrCol = (smoothCorr >= 0.0f) ? MauveTheme::amberBright : juce::Colour::fromRGB(255, 75, 55);
    g.setColour(corrCol.withAlpha(0.35f));
    float barLeft = std::min(centerCorrX, markerX + 1.5f);
    float barW    = std::abs((markerX + 1.5f) - centerCorrX);
    g.fillRect(barLeft, midY - 2.0f, barW, 4.0f);
    g.setColour(corrCol);
    g.fillRect(markerX, midY - 2.5f, 3.0f, 5.0f);
    startX += corrTrackW + 8.0f;

    g.setColour(corrCol);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    juce::String corrStr = (smoothCorr >= 0.0f ? "+" : "") + juce::String(smoothCorr, 2);
    g.drawText(corrStr, juce::Rectangle<float>(startX, (float)area.getY(), 40.0f, (float)area.getHeight()), juce::Justification::centredLeft, false);

    // Right: Active Signal Chain Readout, Latency & Brand
    float rightEdge = (float)area.getRight() - 14.0f;
    g.setColour(MauveTheme::amber);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("MAUVE ZONE", juce::Rectangle<float>(rightEdge - 120.0f, (float)area.getY(), 120.0f, (float)area.getHeight()),
               juce::Justification::centredRight, false);

    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    juce::String latStr = "LATENCY: " + juce::String(processorRef.getLatencySamples()) + " SPL";
    g.drawText(latStr, juce::Rectangle<float>(rightEdge - 245.0f, (float)area.getY(), 115.0f, (float)area.getHeight()),
               juce::Justification::centredRight, false);
}
