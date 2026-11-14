#include "PluginProcessor.h"
#include "PluginEditor.h"

OxidationDeckAudioProcessor::OxidationDeckAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",   juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

OxidationDeckAudioProcessor::~OxidationDeckAudioProcessor()
{
}

const juce::String OxidationDeckAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OxidationDeckAudioProcessor::acceptsMidi() const
{
    return false;
}

bool OxidationDeckAudioProcessor::producesMidi() const
{
    return false;
}

bool OxidationDeckAudioProcessor::isMidiEffect() const
{
    return false;
}

double OxidationDeckAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

namespace
{
    static constexpr const char* kFactoryPresetNames[5] = {
        "01: MASTER // 30 IPS TUBE GLUE",
        "02: MASTER // VCA SNAP & IRON",
        "03: MASTER // GERMANIUM VELVET",
        "04: STEM // DRUM REEL PUNCH",
        "05: EXPERIMENTAL // OXIDE SHED"
    };
}

int OxidationDeckAudioProcessor::getNumPrograms()
{
    return 5;
}

int OxidationDeckAudioProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void OxidationDeckAudioProcessor::setCurrentProgram(int index)
{
    int clamped = juce::jlimit(0, 4, index);
    if (clamped != currentProgramIndex)
        applyFactoryPreset(clamped);
}

const juce::String OxidationDeckAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < 5)
        return kFactoryPresetNames[index];
    return {};
}

void OxidationDeckAudioProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/)
{
}

void OxidationDeckAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    baseSampleRate = sampleRate;
    baseBlockSize = samplesPerBlock;

    // Setup Oversamplers (2x = factor 1, 4x = factor 2, 8x = factor 3)
    for (size_t i = 0; i < 3; ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, i + 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        oversamplers[i]->initProcessing(static_cast<size_t>(samplesPerBlock));
        oversamplers[i]->reset();
    }

    // Prepare bypass delay line (latency-matched pass-through)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    bypassDelay.prepare(spec);
    bypassDelay.reset();

    // Prepare Preamp, Dynamics & Tape Stages
    double oversampledRate = baseSampleRate * (1 << (currentOversampleIndex + 1));
    int oversampledBlock = baseBlockSize * (1 << (currentOversampleIndex + 1));
    preampStage.prepare(oversampledRate, oversampledBlock);
    dynamicsStage.prepare(oversampledRate, oversampledBlock);
    tapeStage.prepare(oversampledRate, oversampledBlock);

    // Prepare Deck 4 Limiter (Master-bus baseband true-peak)
    limiterStage.prepare(baseSampleRate, baseBlockSize);

    updateOversamplingChoice();
    if (oversamplers[currentOversampleIndex])
    {
        int totalLat = juce::roundToInt(oversamplers[currentOversampleIndex]->getLatencyInSamples())
                     + limiterStage.getLatencySamples();
        setLatencySamples(totalLat);
        bypassDelay.setDelay(static_cast<float>(totalLat));
    }
}

void OxidationDeckAudioProcessor::releaseResources()
{
    for (auto& os : oversamplers)
    {
        if (os)
            os->reset();
    }
    bypassDelay.reset();
    preampStage.reset();
    dynamicsStage.reset();
    tapeStage.reset();
    limiterStage.reset();
}

bool OxidationDeckAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void OxidationDeckAudioProcessor::updateOversamplingChoice()
{
    auto* param = apvts.getRawParameterValue(IDs::oversample.getParamID());
    int choice = param ? juce::roundToInt(param->load()) : 1;
    choice = juce::jlimit(0, 2, choice);

    if (choice != currentOversampleIndex || oversamplers[choice] == nullptr)
    {
        currentOversampleIndex = choice;
        if (oversamplers[currentOversampleIndex])
        {
            oversamplers[currentOversampleIndex]->reset();
            int totalLat = juce::roundToInt(oversamplers[currentOversampleIndex]->getLatencyInSamples())
                         + limiterStage.getLatencySamples();
            setLatencySamples(totalLat);
            bypassDelay.setDelay(static_cast<float>(totalLat));

            double oversampledRate = baseSampleRate * (1 << (currentOversampleIndex + 1));
            preampStage.setSampleRate(oversampledRate);
            dynamicsStage.setSampleRate(oversampledRate);
            tapeStage.setSampleRate(oversampledRate);
        }
    }
}

void OxidationDeckAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumSamples() == 0 || totalNumInputChannels < 2)
        return;

    const int numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    // 1. Measure Input Levels & Phase Correlation
    float peakInL = 0.0f;
    float peakInR = 0.0f;
    double sumLR = 0.0;
    double sumLL = 0.0;
    double sumRR = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = leftChannel[i];
        float r = rightChannel[i];
        peakInL = juce::jmax(peakInL, std::abs(l));
        peakInR = juce::jmax(peakInR, std::abs(r));
        sumLR += (double)(l * r);
        sumLL += (double)(l * l);
        sumRR += (double)(r * r);
    }
    inPeakL.store(peakInL, std::memory_order_relaxed);
    inPeakR.store(peakInR, std::memory_order_relaxed);

    double denom = std::sqrt(sumLL * sumRR);
    float corr = (denom > 1e-9) ? (float)(sumLR / denom) : 1.0f;
    correlationVal.store(corr, std::memory_order_relaxed);

    // 2. Master Bypass Check
    auto* bypassParam = apvts.getRawParameterValue(IDs::bypass.getParamID());
    bool isBypassed = (bypassParam != nullptr && bypassParam->load() > 0.5f);

    updateOversamplingChoice();
    auto& activeOversampler = oversamplers[currentOversampleIndex];
    auto* d4BypassP = apvts.getRawParameterValue(IDs::deck4Bypass.getParamID());
    bool d4Bypassed = d4BypassP ? (d4BypassP->load() > 0.5f) : false;

    if (isBypassed)
    {
        // Delay compensation during bypass maintains phase alignment with host mix
        juce::dsp::AudioBlock<float> block(buffer);
        bypassDelay.process(juce::dsp::ProcessContextReplacing<float>(block));

        outPeakL.store(peakInL, std::memory_order_relaxed);
        outPeakR.store(peakInR, std::memory_order_relaxed);
        return;
    }

    // 3. Stereo ⇄ Mid/Side (M/S) Matrix Encoding
    auto* routingParam = apvts.getRawParameterValue(IDs::routingMode.getParamID());
    bool isMidSide = (routingParam != nullptr && routingParam->load() > 0.5f);

    const float invSqrt2 = 0.7071067811865475f;
    if (isMidSide)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float l = leftChannel[i];
            float r = rightChannel[i];
            leftChannel[i]  = (l + r) * invSqrt2; // Mid
            rightChannel[i] = (l - r) * invSqrt2; // Side
        }
    }

    // 4. Multirate Oversampling (2x, 4x, 8x) & Dynamic 3-Deck Signal Chain Routing
    juce::dsp::AudioBlock<float> block(buffer);
    if (activeOversampler)
    {
        auto oversampledBlock = activeOversampler->processSamplesUp(block);

        auto* chainOrderP = apvts.getRawParameterValue(IDs::chainOrder.getParamID());
        int targetOrder = chainOrderP ? juce::jlimit(0, 5, juce::roundToInt(chainOrderP->load())) : 0;
        if (targetOrder != currentChainOrder)
        {
            currentChainOrder = targetOrder;
            preampStage.reset();
            dynamicsStage.reset();
            tapeStage.reset();
            chainTransitionGain = 0.0f; // Smooth fade-in after topology swap
        }

        // Stage execution lambdas (with internal per-sample clickless bypass & parameter smoothing)
        auto runDeck1Preamp = [&]()
        {
            auto* d1BypassP = apvts.getRawParameterValue(IDs::deck1Bypass.getParamID());
            bool d1Bypassed = d1BypassP ? (d1BypassP->load() > 0.5f) : false;

            auto* flavorP   = apvts.getRawParameterValue(IDs::flavor.getParamID());
            auto* driveP    = apvts.getRawParameterValue(IDs::drive.getParamID());
            auto* biasP     = apvts.getRawParameterValue(IDs::bias.getParamID());
            auto* preTiltP  = apvts.getRawParameterValue(IDs::preTilt.getParamID());
            auto* mixP      = apvts.getRawParameterValue(IDs::parallelLine.getParamID());
            auto* autoGainP = apvts.getRawParameterValue(IDs::autoGain.getParamID());

            int flavorIdx   = flavorP ? juce::roundToInt(flavorP->load()) : 0;
            float driveVal  = driveP ? driveP->load() : 0.0f;
            float biasVal   = biasP ? biasP->load() : 50.0f;
            float tiltVal   = preTiltP ? preTiltP->load() : 0.0f;
            float mixVal    = mixP ? mixP->load() : 100.0f;
            bool autoGainOn = autoGainP ? (autoGainP->load() > 0.5f) : false;

            preampStage.process(oversampledBlock, flavorIdx, driveVal, biasVal, tiltVal, mixVal, autoGainOn, d1Bypassed);
        };

        auto runDeck2Dynamics = [&]()
        {
            auto* d2BypassP = apvts.getRawParameterValue(IDs::deck2Bypass.getParamID());
            bool d2Bypassed = d2BypassP ? (d2BypassP->load() > 0.5f) : false;

            auto* compModeP     = apvts.getRawParameterValue(IDs::compMode.getParamID());
            auto* compThreshP   = apvts.getRawParameterValue(IDs::compThresh.getParamID());
            auto* compRatioP    = apvts.getRawParameterValue(IDs::compRatio.getParamID());
            auto* compAttackP   = apvts.getRawParameterValue(IDs::compAttack.getParamID());
            auto* compReleaseP  = apvts.getRawParameterValue(IDs::compRelease.getParamID());
            auto* compScHpfP    = apvts.getRawParameterValue(IDs::compScHpf.getParamID());
            auto* compScListenP = apvts.getRawParameterValue(IDs::compScListen.getParamID());

            int compModeVal      = compModeP ? juce::roundToInt(compModeP->load()) : 0;
            float compThreshVal  = compThreshP ? compThreshP->load() : -14.0f;
            int compRatioVal     = compRatioP ? juce::roundToInt(compRatioP->load()) : 1;
            int compAttackVal    = compAttackP ? juce::roundToInt(compAttackP->load()) : 4;
            int compReleaseVal   = compReleaseP ? juce::roundToInt(compReleaseP->load()) : 4;
            float compScHpfVal   = compScHpfP ? compScHpfP->load() : 85.0f;
            bool compScListenVal = compScListenP ? (compScListenP->load() > 0.5f) : false;

            dynamicsStage.process(oversampledBlock, compModeVal, compThreshVal, compRatioVal,
                                  compAttackVal, compReleaseVal, compScHpfVal, compScListenVal, d2Bypassed);

            grL.store(dynamicsStage.getGainReductionL(), std::memory_order_relaxed);
            grR.store(dynamicsStage.getGainReductionR(), std::memory_order_relaxed);
        };

        auto runDeck3Tape = [&]()
        {
            auto* d3BypassP = apvts.getRawParameterValue(IDs::deck3Bypass.getParamID());
            bool d3Bypassed = d3BypassP ? (d3BypassP->load() > 0.5f) : false;

            auto* tapeSpeedP   = apvts.getRawParameterValue(IDs::tapeSpeed.getParamID());
            auto* tapeSatP     = apvts.getRawParameterValue(IDs::tapeSat.getParamID());
            auto* tapeBiasCalP = apvts.getRawParameterValue(IDs::tapeBiasCal.getParamID());
            auto* tapeFlutterP = apvts.getRawParameterValue(IDs::tapeFlutter.getParamID());
            auto* tapeWearP    = apvts.getRawParameterValue(IDs::tapeWear.getParamID());

            int tapeSpeedVal     = tapeSpeedP ? juce::roundToInt(tapeSpeedP->load()) : 1;
            float tapeSatVal     = tapeSatP ? tapeSatP->load() : 3.8f;
            float tapeBiasCalVal = tapeBiasCalP ? tapeBiasCalP->load() : 1.5f;
            float tapeFlutterVal = tapeFlutterP ? tapeFlutterP->load() : 12.0f;
            float tapeWearVal    = tapeWearP ? tapeWearP->load() : 16.0f;

            tapeStage.process(oversampledBlock, tapeSpeedVal, tapeSatVal, tapeBiasCalVal, tapeFlutterVal, tapeWearVal, d3Bypassed);
        };

        // 6 Permutations of [Preamp (0), Dynamics (1), Tape (2)]
        static constexpr int kOrderTable[6][3] = {
            { 0, 1, 2 }, // 0: PRE  > COMP > TAPE
            { 0, 2, 1 }, // 1: PRE  > TAPE > COMP
            { 1, 0, 2 }, // 2: COMP > PRE  > TAPE
            { 1, 2, 0 }, // 3: COMP > TAPE > PRE
            { 2, 0, 1 }, // 4: TAPE > PRE  > COMP
            { 2, 1, 0 }  // 5: TAPE > COMP > PRE
        };

        for (int step = 0; step < 3; ++step)
        {
            switch (kOrderTable[currentChainOrder][step])
            {
                case 0: runDeck1Preamp();   break;
                case 1: runDeck2Dynamics(); break;
                case 2: runDeck3Tape();     break;
            }
        }

        activeOversampler->processSamplesDown(block);
    }

    // 5. Mid/Side ⇄ Stereo Decode
    if (isMidSide)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float m = leftChannel[i];
            float s = rightChannel[i];
            leftChannel[i]  = (m + s) * invSqrt2; // Left
            rightChannel[i] = (m - s) * invSqrt2; // Right
        }
    }

    // 6. Deck 4: Master-Bus True-Peak Limiter
    auto* ceilingP   = apvts.getRawParameterValue(IDs::limCeiling.getParamID());
    auto* inGainP    = apvts.getRawParameterValue(IDs::limInGain.getParamID());
    auto* lookaheadP = apvts.getRawParameterValue(IDs::limLookahead.getParamID());
    auto* releaseP   = apvts.getRawParameterValue(IDs::limRelease.getParamID());

    float ceilingVal   = ceilingP ? ceilingP->load() : -1.0f;
    float inGainVal    = inGainP ? inGainP->load() : 0.0f;
    float lookaheadVal = lookaheadP ? lookaheadP->load() : 2.0f;
    int releaseVal     = releaseP ? juce::roundToInt(releaseP->load()) : 3;

    limiterStage.process(block, ceilingVal, inGainVal, lookaheadVal, releaseVal, d4Bypassed);

    // 7. Speaker Protection Safety Guard & Output Metering
    const float transitionStep = 1.0f / std::max(32.0f, 0.005f * static_cast<float>(baseSampleRate));
    float peakOutL = 0.0f;
    float peakOutR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        if (chainTransitionGain < 1.0f)
            chainTransitionGain = std::min(1.0f, chainTransitionGain + transitionStep);

        float outL = std::isfinite(leftChannel[i])  ? (leftChannel[i] * chainTransitionGain)  : 0.0f;
        float outR = std::isfinite(rightChannel[i]) ? (rightChannel[i] * chainTransitionGain) : 0.0f;

        // Hard speaker fuse at +6 dBFS (2.0 linear) in case Deck 4 Limiter is bypassed with extreme drive
        outL = juce::jlimit(-2.0f, 2.0f, outL);
        outR = juce::jlimit(-2.0f, 2.0f, outR);

        leftChannel[i]  = outL;
        rightChannel[i] = outR;

        peakOutL = juce::jmax(peakOutL, std::abs(outL));
        peakOutR = juce::jmax(peakOutR, std::abs(outR));
    }
    outPeakL.store(peakOutL, std::memory_order_relaxed);
    outPeakR.store(peakOutR, std::memory_order_relaxed);
}

bool OxidationDeckAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OxidationDeckAudioProcessor::createEditor()
{
    return new OxidationDeckAudioProcessorEditor(*this);
}

void OxidationDeckAudioProcessor::applyFactoryPreset(int programIndex)
{
    programIndex = juce::jlimit(0, 4, programIndex);
    currentProgramIndex = programIndex;
    apvts.state.setProperty("preset_index", currentProgramIndex, nullptr);

    auto setParam = [this](const juce::ParameterID& id, float plainVal)
    {
        if (auto* p = apvts.getParameter(id.getParamID()))
        {
            const float normVal = p->convertTo0to1(plainVal);
            p->beginChangeGesture();
            p->setValueNotifyingHost(normVal);
            p->endChangeGesture();
        }
    };

    // Ensure all decks and master bus are active by default on preset load
    setParam(IDs::bypass, 0.0f);
    setParam(IDs::deck1Bypass, 0.0f);
    setParam(IDs::deck2Bypass, 0.0f);
    setParam(IDs::deck3Bypass, 0.0f);
    setParam(IDs::deck4Bypass, 0.0f);
    setParam(IDs::compScListen, 0.0f);
    setParam(IDs::autoGain, 1.0f);

    const int presetId = programIndex + 1;
    switch (presetId)
    {
        case 1: // 01: MASTER // 30 IPS TUBE GLUE
            setParam(IDs::chainOrder, 0.0f);  // PRE > COMP > TAPE
            setParam(IDs::flavor, 0.0f);      // Triode Class-A
            setParam(IDs::drive, 3.2f);
            setParam(IDs::bias, 65.0f);
            setParam(IDs::preTilt, 0.4f);
            setParam(IDs::parallelLine, 100.0f);
            setParam(IDs::compMode, 1.0f);    // Vari-Mu
            setParam(IDs::compThresh, -14.0f);
            setParam(IDs::compRatio, 0.0f);   // 1.5:1
            setParam(IDs::compAttack, 4.0f);  // 10 ms
            setParam(IDs::compRelease, 4.0f); // Auto
            setParam(IDs::compScHpf, 85.0f);
            setParam(IDs::tapeSpeed, 1.0f);   // 30 IPS
            setParam(IDs::tapeSat, 3.8f);
            setParam(IDs::tapeBiasCal, 1.5f);
            setParam(IDs::tapeFlutter, 10.0f);
            setParam(IDs::tapeWear, 14.0f);
            setParam(IDs::limCeiling, -1.0f);
            setParam(IDs::limInGain, 0.5f);
            setParam(IDs::limLookahead, 2.0f);
            setParam(IDs::limRelease, 3.0f);  // Adaptive
            break;

        case 2: // 02: MASTER // VCA SNAP & IRON (Compressor before Preamp)
            setParam(IDs::chainOrder, 2.0f);  // COMP > PRE > TAPE
            setParam(IDs::flavor, 3.0f);      // Iron Core (Neve)
            setParam(IDs::drive, 4.5f);
            setParam(IDs::bias, 55.0f);
            setParam(IDs::preTilt, 0.8f);
            setParam(IDs::parallelLine, 90.0f);
            setParam(IDs::compMode, 0.0f);    // VCA
            setParam(IDs::compThresh, -16.0f);
            setParam(IDs::compRatio, 1.0f);   // 2:1
            setParam(IDs::compAttack, 5.0f);  // 30 ms
            setParam(IDs::compRelease, 0.0f); // 100 ms
            setParam(IDs::compScHpf, 120.0f);
            setParam(IDs::tapeSpeed, 1.0f);   // 30 IPS
            setParam(IDs::tapeSat, 3.0f);
            setParam(IDs::tapeBiasCal, 2.0f);
            setParam(IDs::tapeFlutter, 6.0f);
            setParam(IDs::tapeWear, 10.0f);
            setParam(IDs::limCeiling, -0.8f);
            setParam(IDs::limInGain, 1.0f);
            setParam(IDs::limLookahead, 1.5f);
            setParam(IDs::limRelease, 0.0f);  // Fast
            break;

        case 3: // 03: MASTER // GERMANIUM VELVET (Pre > Tape > Comp)
            setParam(IDs::chainOrder, 1.0f);  // PRE > TAPE > COMP
            setParam(IDs::flavor, 7.0f);      // Germanium (TG)
            setParam(IDs::drive, 5.2f);
            setParam(IDs::bias, 70.0f);
            setParam(IDs::preTilt, -0.6f);
            setParam(IDs::parallelLine, 85.0f);
            setParam(IDs::compMode, 1.0f);    // Vari-Mu
            setParam(IDs::compThresh, -15.5f);
            setParam(IDs::compRatio, 1.0f);   // 2:1
            setParam(IDs::compAttack, 3.0f);  // 3 ms
            setParam(IDs::compRelease, 4.0f); // Auto
            setParam(IDs::compScHpf, 85.0f);
            setParam(IDs::tapeSpeed, 0.0f);   // 15 IPS
            setParam(IDs::tapeSat, 4.8f);
            setParam(IDs::tapeBiasCal, 0.8f);
            setParam(IDs::tapeFlutter, 16.0f);
            setParam(IDs::tapeWear, 24.0f);
            setParam(IDs::limCeiling, -1.0f);
            setParam(IDs::limInGain, 0.8f);
            setParam(IDs::limLookahead, 2.5f);
            setParam(IDs::limRelease, 3.0f);  // Adaptive
            break;

        case 4: // 04: STEM // DRUM REEL PUNCH (Tape > Comp > Pre)
            setParam(IDs::chainOrder, 5.0f);  // TAPE > COMP > PRE
            setParam(IDs::flavor, 4.0f);      // Nickel Core (API)
            setParam(IDs::drive, 6.5f);
            setParam(IDs::bias, 50.0f);
            setParam(IDs::preTilt, 1.2f);
            setParam(IDs::parallelLine, 75.0f);
            setParam(IDs::compMode, 0.0f);    // VCA
            setParam(IDs::compThresh, -18.0f);
            setParam(IDs::compRatio, 2.0f);   // 4:1
            setParam(IDs::compAttack, 4.0f);  // 10 ms
            setParam(IDs::compRelease, 0.0f); // 100 ms
            setParam(IDs::compScHpf, 120.0f);
            setParam(IDs::tapeSpeed, 0.0f);   // 15 IPS (38 Hz head bump!)
            setParam(IDs::tapeSat, 6.5f);
            setParam(IDs::tapeBiasCal, 0.0f);
            setParam(IDs::tapeFlutter, 14.0f);
            setParam(IDs::tapeWear, 20.0f);
            setParam(IDs::limCeiling, -0.5f);
            setParam(IDs::limInGain, 1.5f);
            setParam(IDs::limLookahead, 1.0f);
            setParam(IDs::limRelease, 0.0f);  // Fast
            break;

        case 5: // 05: EXPERIMENTAL // OXIDE SHED
            setParam(IDs::chainOrder, 4.0f);  // TAPE > PRE > COMP
            setParam(IDs::flavor, 10.0f);     // Oxide Rot & Breakdown
            setParam(IDs::drive, 8.5f);
            setParam(IDs::bias, 80.0f);
            setParam(IDs::preTilt, -1.8f);
            setParam(IDs::parallelLine, 65.0f);
            setParam(IDs::compMode, 1.0f);    // Vari-Mu
            setParam(IDs::compThresh, -20.0f);
            setParam(IDs::compRatio, 2.0f);   // 4:1
            setParam(IDs::compAttack, 2.0f);  // 1 ms
            setParam(IDs::compRelease, 1.0f); // 300 ms
            setParam(IDs::compScHpf, 60.0f);
            setParam(IDs::tapeSpeed, 0.0f);   // 15 IPS
            setParam(IDs::tapeSat, 9.0f);
            setParam(IDs::tapeBiasCal, -1.5f);
            setParam(IDs::tapeFlutter, 42.0f);
            setParam(IDs::tapeWear, 58.0f);
            setParam(IDs::limCeiling, -1.0f);
            setParam(IDs::limInGain, 0.0f);
            setParam(IDs::limLookahead, 3.0f);
            setParam(IDs::limRelease, 3.0f);
            break;

        default:
            break;
    }
}

void OxidationDeckAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    apvts.state.setProperty("preset_index", currentProgramIndex, nullptr);
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OxidationDeckAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        currentProgramIndex = juce::jlimit(0, 4, static_cast<int>(apvts.state.getProperty("preset_index", 0)));
        if (auto* chainOrderP = apvts.getRawParameterValue(IDs::chainOrder.getParamID()))
            currentChainOrder = juce::jlimit(0, 5, juce::roundToInt(chainOrderP->load()));
        preampStage.reset();
        dynamicsStage.reset();
        tapeStage.reset();
        limiterStage.reset();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OxidationDeckAudioProcessor();
}
