#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace IDs
{
    // Global Header
    inline const juce::ParameterID bypass        { "bypass", 1 };
    inline const juce::ParameterID oversample    { "oversample", 1 };
    inline const juce::ParameterID routingMode   { "routing_mode", 1 };
    inline const juce::ParameterID chainOrder    { "chain_order", 1 };

    // Deck 1: Preamp / Line Stage
    inline const juce::ParameterID deck1Bypass   { "deck1_bypass", 1 };
    inline const juce::ParameterID flavor        { "flavor", 1 };
    inline const juce::ParameterID drive         { "drive", 1 };
    inline const juce::ParameterID bias          { "bias", 1 };
    inline const juce::ParameterID preTilt       { "pre_tilt", 1 };
    inline const juce::ParameterID parallelLine  { "parallel_line", 1 };
    inline const juce::ParameterID autoGain      { "auto_gain", 1 };

    // Deck 2: Master Bus Dynamics
    inline const juce::ParameterID deck2Bypass   { "deck2_bypass", 1 };
    inline const juce::ParameterID compMode      { "comp_mode", 1 };
    inline const juce::ParameterID compThresh    { "comp_thresh", 1 };
    inline const juce::ParameterID compRatio     { "comp_ratio", 1 };
    inline const juce::ParameterID compAttack    { "comp_attack", 1 };
    inline const juce::ParameterID compRelease   { "comp_release", 1 };
    inline const juce::ParameterID compScHpf     { "comp_sc_hpf", 1 };
    inline const juce::ParameterID compScListen  { "comp_sc_listen", 1 };

    // Deck 3: Tape Reel
    inline const juce::ParameterID deck3Bypass   { "deck3_bypass", 1 };
    inline const juce::ParameterID tapeSpeed     { "tape_speed", 1 };
    inline const juce::ParameterID tapeSat       { "tape_sat", 1 };
    inline const juce::ParameterID tapeBiasCal   { "tape_bias_cal", 1 };
    inline const juce::ParameterID tapeFlutter   { "tape_flutter", 1 };
    inline const juce::ParameterID tapeWear      { "tape_wear", 1 };

    // Deck 4: True-Peak Limiter
    inline const juce::ParameterID deck4Bypass   { "deck4_bypass", 1 };
    inline const juce::ParameterID limCeiling    { "lim_ceiling", 1 };
    inline const juce::ParameterID limInGain     { "lim_in_gain", 1 };
    inline const juce::ParameterID limLookahead  { "lim_lookahead", 1 };
    inline const juce::ParameterID limRelease    { "lim_release", 1 };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Global ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::bypass, "Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::oversample, "Oversampling",
        juce::StringArray { "2x", "4x", "8x" }, 1)); // Default 4x

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::routingMode, "Routing Mode",
        juce::StringArray { "Stereo", "M/S" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::chainOrder, "Signal Chain Order",
        juce::StringArray {
            "PRE > COMP > TAPE",
            "PRE > TAPE > COMP",
            "COMP > PRE > TAPE",
            "COMP > TAPE > PRE",
            "TAPE > PRE > COMP",
            "TAPE > COMP > PRE"
        }, 0));

    // --- Deck 1: Preamp / Line ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::deck1Bypass, "Preamp Bypass", false));

    const juce::StringArray flavors {
        "Triode Class-A", "Pentode Push-Pull", "Starved Triode",
        "Iron Core (Neve)", "Nickel Core (API)", "Steel Inductor",
        "Class-A Silicon", "Germanium (TG)", "FET Driver",
        "Zener Soft-Clipper", "Oxide Rot"
    };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::flavor, "Circuit Flavor", flavors, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::drive, "Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 3.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::bias, "Bias",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 65.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::preTilt, "Pre-Tilt",
        juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::parallelLine, "Line Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::autoGain, "Auto Gain", true));

    // --- Deck 2: Dynamics ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::deck2Bypass, "Dynamics Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::compMode, "Compressor Mode",
        juce::StringArray { "VCA", "Vari-Mu" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::compThresh, "Threshold",
        juce::NormalisableRange<float>(-30.0f, 10.0f, 0.1f), -14.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::compRatio, "Ratio",
        juce::StringArray { "1.5:1", "2:1", "4:1", "10:1" }, 1));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::compAttack, "Attack",
        juce::StringArray { "0.1 ms", "0.3 ms", "1.0 ms", "3.0 ms", "10 ms", "30 ms" }, 4));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::compRelease, "Release",
        juce::StringArray { "100 ms", "300 ms", "600 ms", "1.2 s", "Auto" }, 4));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::compScHpf, "SC-HPF",
        juce::NormalisableRange<float>(30.0f, 250.0f, 1.0f, 0.5f), 85.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::compScListen, "SC Listen", false));

    // --- Deck 3: Tape Reel ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::deck3Bypass, "Tape Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::tapeSpeed, "Tape Speed",
        juce::StringArray { "15 IPS", "30 IPS" }, 1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::tapeSat, "Tape Saturation",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f), 3.8f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::tapeBiasCal, "Bias Cal",
        juce::NormalisableRange<float>(-3.0f, 3.0f, 0.1f), 1.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::tapeFlutter, "Wow & Flutter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 12.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::tapeWear, "Oxide Wear",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 16.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // --- Deck 4: True-Peak Limiter ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        IDs::deck4Bypass, "Limiter Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::limCeiling, "Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.05f), -1.0f,
        juce::AudioParameterFloatAttributes().withLabel("dBTP")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::limInGain, "Input Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        IDs::limLookahead, "Lookahead",
        juce::NormalisableRange<float>(0.5f, 5.0f, 0.1f), 2.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        IDs::limRelease, "Limiter Release",
        juce::StringArray { "Fast", "Medium", "Slow", "Adaptive" }, 3));

    return layout;
}

