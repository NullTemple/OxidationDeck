#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MauveTheme.h"
#include <cmath>

// -------------------------------------------------------------
// Deck 1: Harmonic Spectrum & Transfer Curve CRT Screen
// -------------------------------------------------------------
class Deck1HarmonicScreen : public juce::Component
{
public:
    Deck1HarmonicScreen() = default;

    void setProfile(const juce::String& text, float h1 = 0.85f, float h2 = 0.30f, float h3 = 0.10f, int flavor = 0, float driveNorm = 0.15f, float biasNorm = 0.5f)
    {
        profileText = text;
        harm1 = h1;
        harm2 = h2;
        harm3 = h3;
        flavorIdx = flavor;
        driveAmount = driveNorm;
        biasAmount = biasNorm;
        repaint();
    }

    void setLiveActivity(float activity)
    {
        float clamped = juce::jlimit(0.0f, 1.5f, activity);
        if (std::abs(clamped - livePulse) > 0.01f)
        {
            livePulse = clamped;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. Recessed Screen Well Background
        g.setColour(MauveTheme::amberDarkBg);
        g.fillRoundedRectangle(bounds, 3.0f);

        // 2. CRT Graticule Grid & Scanlines
        g.setColour(MauveTheme::amber.withAlpha(0.06f));
        for (float y = bounds.getY() + 4.0f; y < bounds.getBottom() - 2.0f; y += 3.0f)
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

        // 3. Inset Top Shadow (3D Depth Cutout)
        juce::ColourGradient insetShadow(juce::Colours::black.withAlpha(0.78f), 0.0f, bounds.getY(),
                                         juce::Colours::transparentBlack, 0.0f, bounds.getY() + 9.0f, false);
        g.setGradientFill(insetShadow);
        g.fillRoundedRectangle(bounds, 3.0f);

        // 4. Phosphor Cathode Amber Bloom (Audio-Reactive)
        float bloomAlpha = 0.12f + 0.16f * juce::jlimit(0.0f, 1.0f, livePulse);
        juce::ColourGradient crtBloom(MauveTheme::amber.withAlpha(bloomAlpha), bounds.getCentreX(), bounds.getCentreY(),
                                      juce::Colours::transparentBlack, bounds.getRight(), bounds.getBottom(), true);
        g.setGradientFill(crtBloom);
        g.fillRoundedRectangle(bounds, 3.0f);

        // 5. Well Border & Specular Bottom Rim
        g.setColour(MauveTheme::amberBorder.brighter(0.15f * livePulse));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.12f));
        g.drawLine(bounds.getX() + 3.0f, bounds.getBottom(), bounds.getRight() - 3.0f, bounds.getBottom(), 1.0f);

        // 6. Left: Profile Text & Sub-Readout
        auto textZone = bounds.reduced(8.0f, 3.0f).withTrimmedRight(82.0f);
        g.setColour(MauveTheme::amberBright);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(profileText, textZone.removeFromTop(18.0f), juce::Justification::centredLeft, false);

        g.setColour(MauveTheme::amber.withAlpha(0.75f));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        juce::String subStr = (livePulse > 0.03f)
            ? ("THD DRIVE: " + juce::String(livePulse * 8.5f, 1) + "%")
            : "HARMONIC ANALYZER";
        g.drawText(subStr, textZone, juce::Justification::centredLeft, false);

        // 7. Center-Right: Miniature Nonlinear Transfer Curve Oscilloscope
        float scopeW = 32.0f;
        float scopeH = bounds.getHeight() - 10.0f;
        float scopeX = bounds.getRight() - 80.0f;
        float scopeY = bounds.getY() + 5.0f;

        g.setColour(juce::Colour::fromRGB(22, 14, 4));
        g.fillRoundedRectangle(scopeX, scopeY, scopeW, scopeH, 2.0f);
        g.setColour(MauveTheme::amberBorder.withAlpha(0.6f));
        g.drawRoundedRectangle(scopeX, scopeY, scopeW, scopeH, 2.0f, 0.7f);

        // Crosshair axes
        g.setColour(MauveTheme::amber.withAlpha(0.20f));
        g.drawHorizontalLine(static_cast<int>(scopeY + scopeH * 0.5f), scopeX + 2.0f, scopeX + scopeW - 2.0f);
        g.drawVerticalLine(static_cast<int>(scopeX + scopeW * 0.5f), scopeY + 2.0f, scopeY + scopeH - 2.0f);

        juce::Path curvePath;
        const int pts = 19;
        float gainK = 1.0f + driveAmount * 2.8f + livePulse * 0.8f;
        float asymBias = (biasAmount - 0.5f) * 0.55f;
        for (int i = 0; i < pts; ++i)
        {
            float nx = (static_cast<float>(i) / static_cast<float>(pts - 1)) * 2.0f - 1.0f;
            float xDrive = nx * gainK + asymBias;
            float ny = std::tanh(xDrive) - std::tanh(asymBias);
            if (flavorIdx == 10) // Wavefold for Oxide Rot
                ny = 0.75f * std::tanh(xDrive) - 0.25f * std::sin(xDrive * 2.4f) - 0.5f * std::tanh(asymBias);
            else if (flavorIdx == 0 || flavorIdx == 2) // Tube asymmetry
                ny = std::tanh(xDrive + 0.18f * xDrive * xDrive) - std::tanh(asymBias + 0.18f * asymBias * asymBias);

            ny = juce::jlimit(-1.0f, 1.0f, ny);
            float px = scopeX + 3.0f + (nx * 0.5f + 0.5f) * (scopeW - 6.0f);
            float py = scopeY + scopeH * 0.5f - ny * (scopeH * 0.40f);
            if (i == 0) curvePath.startNewSubPath(px, py);
            else        curvePath.lineTo(px, py);
        }
        g.setColour(MauveTheme::amber.withAlpha(0.35f));
        g.strokePath(curvePath, juce::PathStrokeType(2.5f));
        g.setColour(MauveTheme::amberBright);
        g.strokePath(curvePath, juce::PathStrokeType(1.2f));

        // 8. Right: 5-Bar Harmonic Spectrum Analyzer (H1..H5)
        float barAreaX = bounds.getRight() - 42.0f;
        float barH = bounds.getHeight() - 10.0f;
        float barY = bounds.getY() + 5.0f;
        float barW = 5.5f;
        float gap = 2.2f;

        float pulseBoost = 1.0f + livePulse * 0.45f;
        float bars[5] = {
            juce::jlimit(0.12f, 1.0f, harm1 * pulseBoost),
            juce::jlimit(0.08f, 1.0f, harm2 * pulseBoost),
            juce::jlimit(0.08f, 1.0f, harm3 * pulseBoost),
            juce::jlimit(0.05f, 1.0f, harm2 * 0.58f * pulseBoost),
            juce::jlimit(0.05f, 1.0f, harm3 * 0.48f * pulseBoost)
        };

        for (int b = 0; b < 5; ++b)
        {
            float bx = barAreaX + b * (barW + gap);
            g.setColour(juce::Colour::fromRGB(28, 18, 6));
            g.fillRoundedRectangle(bx, barY, barW, barH, 1.2f);

            float h = barH * bars[b];
            float y = barY + (barH - h);
            juce::Colour col = (b % 2 == 1) ? MauveTheme::amberBright : MauveTheme::amber;
            if (b >= 3) col = col.withAlpha(0.75f);

            g.setColour(col);
            g.fillRoundedRectangle(bx, y, barW, h, 1.2f);
        }
    }

private:
    juce::String profileText = "2ND HARMONIC BLOOM";
    float harm1 = 0.85f;
    float harm2 = 0.30f;
    float harm3 = 0.10f;
    int flavorIdx = 0;
    float driveAmount = 0.15f;
    float biasAmount = 0.5f;
    float livePulse = 0.0f;
};

// -------------------------------------------------------------
// Deck 2: Dual Stereo VU GR Meter Housing (Recessed 3D Smoked Deck)
// -------------------------------------------------------------
class StereoVUMeter : public juce::Component
{
public:
    StereoVUMeter() = default;

    void setGR(float grLeftDb, float grRightDb)
    {
        grL = grLeftDb;
        grR = grRightDb;

        // Peak-hold ballistics for visual transient catch
        float absL = std::abs(grL);
        float absR = std::abs(grR);
        peakHoldL = (absL > peakHoldL) ? absL : peakHoldL * 0.95f;
        peakHoldR = (absR > peakHoldR) ? absR : peakHoldR * 0.95f;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. Smoked Bevel Well Housing
        g.setColour(juce::Colour::fromRGB(10, 7, 16));
        g.fillRoundedRectangle(bounds, 4.0f);

        // 2. Inset Top Shadow
        juce::ColourGradient insetShadow(juce::Colours::black.withAlpha(0.85f), 0.0f, bounds.getY(),
                                         juce::Colours::transparentBlack, 0.0f, bounds.getY() + 10.0f, false);
        g.setGradientFill(insetShadow);
        g.fillRoundedRectangle(bounds, 4.0f);

        // 3. Housing Border & Bottom Rim Highlight
        g.setColour(juce::Colour::fromRGB(60, 41, 80));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.10f));
        g.drawLine(bounds.getX() + 3.0f, bounds.getBottom(), bounds.getRight() - 3.0f, bounds.getBottom(), 1.0f);

        auto inner = bounds.reduced(8.0f, 4.0f);

        auto drawChannelMeter = [&](juce::Rectangle<float> row, const char* name, float grVal, float peakHold)
        {
            auto textRow = row.removeFromTop(13.0f);

            g.setColour(MauveTheme::silkscreen);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(name, textRow, juce::Justification::centredLeft, false);

            // Calibration dB marks in the center
            g.setColour(MauveTheme::silkscreenMuted.withAlpha(0.45f));
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.drawText("-3      -6      -9", textRow, juce::Justification::centred, false);

            g.setColour(std::abs(grVal) > 0.1f ? MauveTheme::amberBright : MauveTheme::amber);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(juce::String(grVal, 1) + " dB", textRow, juce::Justification::centredRight, false);

            auto bar = row.reduced(0.0f, 2.0f);
            g.setColour(juce::Colour::fromRGB(22, 13, 31));
            g.fillRoundedRectangle(bar, 2.5f);
            g.setColour(juce::Colour::fromRGB(46, 30, 64));
            g.drawRoundedRectangle(bar, 2.5f, 0.7f);

            // Scale tick notches inside bar
            for (int t = 1; t < 4; ++t)
            {
                float tx = bar.getX() + bar.getWidth() * (t * 0.25f);
                g.setColour(juce::Colour::fromRGB(50, 34, 68));
                g.drawVerticalLine(static_cast<int>(tx), bar.getY() + 1.0f, bar.getBottom() - 1.0f);
            }

            float norm = juce::jlimit(0.0f, 1.0f, std::abs(grVal) / 12.0f);
            float fillW = bar.getWidth() * norm;
            if (fillW > 0.5f)
            {
                juce::ColourGradient grad(juce::Colour::fromRGB(115, 70, 165), bar.getX(), bar.getY(),
                                          MauveTheme::amber, bar.getRight(), bar.getY(), false);
                g.setGradientFill(grad);
                g.fillRoundedRectangle(bar.getX(), bar.getY(), fillW, bar.getHeight(), 2.5f);
            }

            // Peak-Hold Needle
            float peakNorm = juce::jlimit(0.0f, 1.0f, peakHold / 12.0f);
            if (peakNorm > 0.02f)
            {
                float px = bar.getX() + bar.getWidth() * peakNorm;
                g.setColour(MauveTheme::amberBright);
                g.fillRect(px - 1.0f, bar.getY(), 2.0f, bar.getHeight());
            }
        };

        auto row1 = inner.removeFromTop(inner.getHeight() * 0.5f);
        drawChannelMeter(row1, "GR [L]", grL, peakHoldL);
        drawChannelMeter(inner, "GR [R]", grR, peakHoldR);
    }

private:
    float grL = 0.0f;
    float grR = 0.0f;
    float peakHoldL = 0.0f;
    float peakHoldR = 0.0f;
};

// -------------------------------------------------------------
// Deck 4: EBU R128 Loudness Radar CRT Screen (Recessed 3D Well)
// -------------------------------------------------------------
class Deck4LoudnessRadar : public juce::Component
{
public:
    Deck4LoudnessRadar() = default;

    void setTelemetry(float integrated, float truePeak, float shortTerm, float lra, float grDb = 0.0f)
    {
        lufsInt = integrated;
        tpMax = truePeak;
        lufsSt = shortTerm;
        lraVal = lra;
        limiterGrDb = grDb;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. Recessed Screen Well Background
        g.setColour(MauveTheme::amberDarkBg);
        g.fillRoundedRectangle(bounds, 3.0f);

        // CRT Scanlines
        g.setColour(MauveTheme::amber.withAlpha(0.05f));
        for (float y = bounds.getY() + 3.0f; y < bounds.getBottom() - 2.0f; y += 3.0f)
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

        // 2. Inset Top Shadow
        juce::ColourGradient insetShadow(juce::Colours::black.withAlpha(0.75f), 0.0f, bounds.getY(),
                                         juce::Colours::transparentBlack, 0.0f, bounds.getY() + 8.0f, false);
        g.setGradientFill(insetShadow);
        g.fillRoundedRectangle(bounds, 3.0f);

        // 3. Phosphor Cathode Amber Bloom
        float grIntensity = juce::jlimit(0.0f, 1.0f, std::abs(limiterGrDb) / 6.0f);
        juce::ColourGradient crtBloom(MauveTheme::amber.withAlpha(0.14f + 0.14f * grIntensity),
                                      bounds.getCentreX(), bounds.getCentreY(),
                                      juce::Colours::transparentBlack, bounds.getRight(), bounds.getBottom(), true);
        g.setGradientFill(crtBloom);
        g.fillRoundedRectangle(bounds, 3.0f);

        // 4. Well Border & Specular Highlight
        g.setColour(MauveTheme::amberBorder.brighter(0.2f * grIntensity));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.12f));
        g.drawLine(bounds.getX() + 3.0f, bounds.getBottom(), bounds.getRight() - 3.0f, bounds.getBottom(), 1.0f);

        auto inner = bounds.reduced(6.0f, 4.0f);

        // Top half: Dual main readouts (INTEGRATED & TRUE-PEAK) with Center Vertical GR Attenuation Column
        auto topHalf  = inner.removeFromTop(inner.getHeight() * 0.66f);
        float centerX = topHalf.getCentreX();
        auto leftCol  = topHalf.removeFromLeft(topHalf.getWidth() * 0.45f);
        auto rightCol = topHalf.removeFromRight(topHalf.getWidth() * 0.82f);

        // Center Vertical GR Bar Track
        juce::Rectangle<float> grBar(centerX - 3.5f, topHalf.getY() + 2.0f, 7.0f, topHalf.getHeight() - 4.0f);
        g.setColour(juce::Colour::fromRGB(26, 16, 4));
        g.fillRoundedRectangle(grBar, 2.0f);
        g.setColour(MauveTheme::amberBorder);
        g.drawRoundedRectangle(grBar, 2.0f, 0.7f);

        if (grIntensity > 0.01f)
        {
            float grFillH = grBar.getHeight() * grIntensity;
            g.setColour(MauveTheme::amberBright);
            g.fillRoundedRectangle(grBar.getX() + 1.0f, grBar.getY() + 1.0f, grBar.getWidth() - 2.0f, grFillH, 1.5f);
        }

        // Separator line between top and bottom
        g.setColour(MauveTheme::amberBorder);
        g.drawLine(inner.getX(), topHalf.getBottom(), inner.getRight(), topHalf.getBottom(), 1.0f);

        // Integrated LUFS
        g.setColour(MauveTheme::amber);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("INTEGRATED", leftCol.removeFromTop(12.0f), juce::Justification::centred, false);

        g.setColour(MauveTheme::amberBright);
        g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
        juce::String intStr = (lufsInt <= -69.0f) ? "-INF" : juce::String(lufsInt, 1);
        g.drawText(intStr, leftCol.removeFromTop(18.0f), juce::Justification::centred, false);

        g.setColour(MauveTheme::amberBright.withAlpha(0.70f));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText("LUFS", leftCol, juce::Justification::centred, false);

        // True-Peak dBTP
        g.setColour(MauveTheme::amber);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("TRUE-PEAK", rightCol.removeFromTop(12.0f), juce::Justification::centred, false);

        g.setColour(MauveTheme::amberBright);
        g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
        juce::String tpStr = (tpMax <= -59.0f) ? "-INF" : juce::String(tpMax, 2);
        g.drawText(tpStr, rightCol.removeFromTop(18.0f), juce::Justification::centred, false);

        g.setColour(MauveTheme::amberBright.withAlpha(0.70f));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText("dBTP", rightCol, juce::Justification::centred, false);

        // Bottom row: SHORT-TERM, GR, & LRA
        auto botRow = inner;
        botRow.removeFromTop(3.0f);

        g.setColour(MauveTheme::amber);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        juce::String stStr = (lufsSt <= -69.0f) ? "ST: -INF" : ("ST: " + juce::String(lufsSt, 1));
        g.drawText(stStr, botRow.removeFromLeft(botRow.getWidth() * 0.36f),
                   juce::Justification::centredLeft, false);

        g.setColour(grIntensity > 0.02f ? MauveTheme::amberBright : MauveTheme::amber.withAlpha(0.8f));
        g.drawText("GR: " + juce::String(limiterGrDb, 1) + "dB", botRow.removeFromLeft(botRow.getWidth() * 0.55f),
                   juce::Justification::centred, false);

        g.setColour(MauveTheme::amber);
        g.drawText("LRA: " + juce::String(lraVal, 1), botRow, juce::Justification::centredRight, false);
    }

private:
    float lufsInt     = -14.1f;
    float tpMax       = -1.00f;
    float lufsSt      = -12.8f;
    float lraVal      = 5.2f;
    float limiterGrDb = 0.0f;
};
