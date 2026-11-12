#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MauveTheme.h"

class TapeTransportComponent : public juce::Component, private juce::Timer
{
public:
    TapeTransportComponent();
    ~TapeTransportComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setRunning(bool isRunning);
    void setSpeedIPS(float ips);
    void setFluxIntensity(float intensity);

private:
    void timerCallback() override;

    float leftReelAngle   = 0.0f;
    float rightReelAngle  = 0.0f;
    bool running          = true;
    float speedMultiplier = 1.0f;

    // Live audio-reactive flux smoothing for 60 FPS visual bloom
    float targetFlux  = 0.0f;
    float currentFlux = 0.0f;

    void drawBadassReel(juce::Graphics& g, float cx, float cy, float diameter, float angle, float tapePackFraction);
    void drawHeadBlock(juce::Graphics& g, float cx, float cy, float w, float h, float flux);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapeTransportComponent)
};
