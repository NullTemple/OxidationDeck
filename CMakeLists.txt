cmake_minimum_required(VERSION 3.22)

project(MZOD1 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Standard official JUCE link to local C:/JUCE installation without duplicating files
add_subdirectory("C:/JUCE" "${CMAKE_CURRENT_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

juce_add_plugin(MZOD1
    COMPANY_NAME "Mauve Zone"
    BUNDLE_ID "com.mauvezone.mzod1"
    PLUGIN_MANUFACTURER_CODE Mauv
    PLUGIN_CODE Mzd1
    FORMATS VST3 Standalone
    PRODUCT_NAME "The Oxidation Deck"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(MZOD1
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Parameters.h
        Source/DSP/PreampStage.h
        Source/DSP/DynamicsStage.h
        Source/DSP/TapeStage.h
        Source/DSP/LimiterStage.h
        Source/UI/MauveTheme.h
        Source/UI/MauveLookAndFeel.h
        Source/UI/MauveLookAndFeel.cpp
        Source/UI/LabeledKnob.h
        Source/UI/TapeTransportComponent.h
        Source/UI/TapeTransportComponent.cpp
        Source/UI/CrtDisplays.h
        Source/UI/Deck1Preamp.h
        Source/UI/Deck1Preamp.cpp
        Source/UI/Deck2Dynamics.h
        Source/UI/Deck2Dynamics.cpp
        Source/UI/Deck3Tape.h
        Source/UI/Deck3Tape.cpp
        Source/UI/Deck4Limiter.h
        Source/UI/Deck4Limiter.cpp)

target_compile_definitions(MZOD1
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MZOD1
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
