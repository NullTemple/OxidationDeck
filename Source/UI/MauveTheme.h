#pragma once

#include <juce_graphics/juce_graphics.h>

namespace MauveTheme
{
    // Palette directly from FRONT_PANEL_MOCKUP.html
    inline const juce::Colour spaceBg         = juce::Colour::fromRGB(6, 5, 8);
    inline const juce::Colour chassisOuter1   = juce::Colour::fromRGB(24, 17, 34);
    inline const juce::Colour chassisOuter2   = juce::Colour::fromRGB(12, 8, 18);
    inline const juce::Colour chassisBorder   = juce::Colour::fromRGB(56, 38, 74);
    inline const juce::Colour chassisInnerBg  = juce::Colour::fromRGB(10, 7, 18);
    inline const juce::Colour chassisInnerBrd = juce::Colour::fromRGB(38, 24, 55);

    // Header & Footer
    inline const juce::Colour headerBg        = juce::Colour::fromRGB(21, 14, 31);
    inline const juce::Colour headerBorder    = juce::Colour::fromRGB(48, 31, 68);
    inline const juce::Colour footerBg        = juce::Colour::fromRGB(18, 11, 26);

    // Deck Surfaces
    inline const juce::Colour deckConsole1    = juce::Colour::fromRGB(28, 19, 38);
    inline const juce::Colour deckConsole2    = juce::Colour::fromRGB(17, 11, 24);
    inline const juce::Colour deckConsoleBrd  = juce::Colour::fromRGB(61, 41, 82);

    inline const juce::Colour deckComp1       = juce::Colour::fromRGB(22, 15, 32);
    inline const juce::Colour deckComp2       = juce::Colour::fromRGB(11, 7, 17);
    inline const juce::Colour deckCompBrd     = juce::Colour::fromRGB(52, 34, 71);

    inline const juce::Colour deckTape1       = juce::Colour::fromRGB(34, 22, 48);
    inline const juce::Colour deckTape2       = juce::Colour::fromRGB(20, 13, 30);
    inline const juce::Colour deckTapeBrd     = juce::Colour::fromRGB(74, 50, 94);

    inline const juce::Colour deckLimiter1    = juce::Colour::fromRGB(19, 12, 26);
    inline const juce::Colour deckLimiter2    = juce::Colour::fromRGB(9, 5, 13);
    inline const juce::Colour deckLimiterBrd  = juce::Colour::fromRGB(46, 29, 62);

    // Silkscreen & Text
    inline const juce::Colour silkscreen      = juce::Colour::fromRGB(191, 174, 201);
    inline const juce::Colour silkscreenMuted = juce::Colour::fromRGB(140, 122, 152);
    inline const juce::Colour textBright      = juce::Colour::fromRGB(245, 240, 250);

    // Studio Amber Phosphor
    inline const juce::Colour amber           = juce::Colour::fromRGB(255, 170, 0);
    inline const juce::Colour amberWarm       = juce::Colour::fromRGB(255, 148, 0);
    inline const juce::Colour amberBright     = juce::Colour::fromRGB(255, 212, 128);
    inline const juce::Colour amberDarkBg     = juce::Colour::fromRGB(14, 9, 2);
    inline const juce::Colour amberBorder     = juce::Colour::fromRGB(90, 60, 0);

    // Modulation Halo Accent
    inline const juce::Colour cyanMod         = juce::Colour::fromRGB(56, 189, 248);

    // Hardware Buttons
    inline const juce::Colour btnBg           = juce::Colour::fromRGB(23, 16, 32);
    inline const juce::Colour btnBorder       = juce::Colour::fromRGB(60, 42, 79);
    inline const juce::Colour btnActiveBg     = juce::Colour::fromRGB(58, 37, 80);
}

