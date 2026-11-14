#include "TapeTransportComponent.h"
#include "MauveLookAndFeel.h"
#include <cmath>

TapeTransportComponent::TapeTransportComponent()
{
    startTimerHz(60);
}

TapeTransportComponent::~TapeTransportComponent()
{
    stopTimer();
}

void TapeTransportComponent::setRunning(bool isRunning)
{
    running = isRunning;
}

void TapeTransportComponent::setSpeedIPS(float ips)
{
    speedMultiplier = ips / 30.0f;
}

void TapeTransportComponent::setFluxIntensity(float intensity)
{
    targetFlux = juce::jlimit(0.0f, 2.0f, intensity);
}

void TapeTransportComponent::timerCallback()
{
    if (running)
    {
        // Reel rotation speed (30 IPS vs 15 IPS, both clockwise)
        leftReelAngle  += 0.024f * speedMultiplier;
        rightReelAngle += 0.027f * speedMultiplier;

        if (leftReelAngle > juce::MathConstants<float>::twoPi)
            leftReelAngle -= juce::MathConstants<float>::twoPi;
        if (rightReelAngle > juce::MathConstants<float>::twoPi)
            rightReelAngle -= juce::MathConstants<float>::twoPi;
    }

    // Ballistics smoothing for live flux core glow
    if (targetFlux > currentFlux)
        currentFlux = currentFlux * 0.45f + targetFlux * 0.55f; // Fast, punchy attack
    else
        currentFlux = currentFlux * 0.88f + targetFlux * 0.12f; // Warm analog phosphor decay

    repaint();
}

void TapeTransportComponent::resized()
{
}

void TapeTransportComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float midY = bounds.getCentreY();

    // 1. Recessed Stage Housing
    g.setColour(juce::Colour::fromRGB(15, 9, 23));
    g.fillRoundedRectangle(bounds, 4.0f);

    // 2. Inset Top & Edge Shadow
    juce::ColourGradient insetShadow(juce::Colours::black.withAlpha(0.92f), 0.0f, bounds.getY(),
                                     juce::Colours::transparentBlack, 0.0f, bounds.getY() + 12.0f, false);
    g.setGradientFill(insetShadow);
    g.fillRoundedRectangle(bounds, 4.0f);

    // 3. Stage Border & Highlights
    g.setColour(juce::Colour::fromRGB(63, 41, 84));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.10f));
    g.drawLine(bounds.getX() + 3.0f, bounds.getBottom() - 1.0f, bounds.getRight() - 3.0f, bounds.getBottom() - 1.0f, 1.0f);

    // 4. Ambient Dynamic Flux Core Glow across the transport deck
    float headW = 54.0f;
    float headH = 42.0f;
    float headX = (width - headW) * 0.5f;
    float headY = midY - headH * 0.5f;
    float headCenterX = headX + headW * 0.5f;
    float headCenterY = headY + headH * 0.5f;

    float glowRadius = 35.0f + 30.0f * currentFlux;
    float glowAlpha = 0.15f + 0.45f * juce::jlimit(0.0f, 1.0f, currentFlux);
    juce::Colour ambientGold = juce::Colour::fromRGB(255, 140, 0).withAlpha(glowAlpha);

    juce::ColourGradient deckHalo(ambientGold, headCenterX, headCenterY,
                                  juce::Colours::transparentBlack, headCenterX, headCenterY + glowRadius, true);
    g.setGradientFill(deckHalo);
    g.fillEllipse(headCenterX - glowRadius, headCenterY - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);

    // 5. Tape Path Ribbon across the deck
    juce::ColourGradient tapeGrad(juce::Colour::fromRGB(65, 42, 22), 0.0f, midY - 2.0f,
                                  juce::Colour::fromRGB(32, 18, 8), 0.0f, midY + 2.0f, false);
    g.setGradientFill(tapeGrad);
    g.fillRect(26.0f, midY - 2.0f, width - 52.0f, 4.0f);

    // Subtle tape edge specular glint
    g.setColour(juce::Colour::fromRGB(140, 95, 55).withAlpha(0.40f));
    g.drawLine(26.0f, midY - 2.0f, width - 26.0f, midY - 2.0f, 0.75f);

    // 6. Left & Right Precision Tension Arms & Rollers
    auto drawTensionArm = [&](float x, bool /*isRight*/)
    {
        // Arm lever body
        g.setColour(juce::Colour::fromRGB(45, 30, 58));
        g.fillRoundedRectangle(x - 2.0f, 10.0f, 4.0f, 26.0f, 2.0f);
        g.setColour(juce::Colour::fromRGB(85, 58, 108));
        g.drawRoundedRectangle(x - 2.0f, 10.0f, 4.0f, 26.0f, 2.0f, 0.8f);

        // Pivot screw
        MauveLookAndFeel::drawChassisScrew(g, x, 13.0f, 6.0f);

        // Polished chrome guide roller
        float rx = x - 5.5f;
        float ry = 32.0f;
        float rd = 11.0f;

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse(rx, ry + 1.5f, rd, rd);

        // Chrome bevel
        juce::ColourGradient rollerGrad(juce::Colour::fromRGB(240, 230, 250), rx + 2.5f, ry + 2.5f,
                                        juce::Colour::fromRGB(60, 42, 78), rx + rd, ry + rd, true);
        g.setGradientFill(rollerGrad);
        g.fillEllipse(rx, ry, rd, rd);

        // Center jewel cap
        g.setColour(juce::Colour::fromRGB(25, 15, 35));
        g.fillEllipse(rx + 3.0f, ry + 3.0f, 5.0f, 5.0f);
        g.setColour(juce::Colour::fromRGB(180, 150, 210));
        g.drawEllipse(rx, ry, rd, rd, 0.8f);
    };

    drawTensionArm(16.0f, false);
    drawTensionArm(width - 16.0f, true);

    // 7. Capstan Shaft & Heavy Rubber Pinch Roller (Right side of center)
    float capstanX = width * 0.725f;
    // Mirror-chrome capstan shaft
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(capstanX - 2.0f, midY - 9.0f, 4.0f, 18.0f, 1.5f);
    juce::ColourGradient capstanGrad(juce::Colours::white, capstanX - 1.0f, midY - 9.0f,
                                     juce::Colour::fromRGB(120, 110, 135), capstanX + 2.0f, midY + 9.0f, false);
    g.setGradientFill(capstanGrad);
    g.fillRoundedRectangle(capstanX - 1.8f, midY - 9.0f, 3.6f, 18.0f, 1.2f);

    // Heavy rubber pinch roller
    float pinchX = capstanX - 9.0f;
    float pinchW = 14.0f;
    float pinchH = 18.0f;
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(pinchX - pinchW * 0.5f, midY - pinchH * 0.5f + 1.0f, pinchW, pinchH, 2.0f);

    juce::ColourGradient pinchGrad(juce::Colour::fromRGB(38, 39, 44), pinchX - pinchW * 0.5f, midY - pinchH * 0.5f,
                                   juce::Colour::fromRGB(12, 13, 16), pinchX + pinchW * 0.5f, midY + pinchH * 0.5f, false);
    g.setGradientFill(pinchGrad);
    g.fillRoundedRectangle(pinchX - pinchW * 0.5f, midY - pinchH * 0.5f, pinchW, pinchH, 2.0f);
    g.setColour(juce::Colour::fromRGB(55, 56, 62));
    g.drawRoundedRectangle(pinchX - pinchW * 0.5f, midY - pinchH * 0.5f, pinchW, pinchH, 2.0f, 0.8f);

    // Bronze brass collar in pinch roller
    g.setColour(juce::Colour::fromRGB(165, 120, 50));
    g.fillEllipse(pinchX - 2.5f, midY - 2.5f, 5.0f, 5.0f);

    // 8. Badass Metal Tape Reels
    // Spool diameter: 58px. Left reel ~72% full, Right reel ~45% full
    float reelDiameter = 58.0f;
    float leftSpoolX = 40.0f;
    float rightSpoolX = width - 40.0f;

    drawBadassReel(g, leftSpoolX, midY, reelDiameter, leftReelAngle, 0.72f);
    drawBadassReel(g, rightSpoolX, midY, reelDiameter, rightReelAngle, 0.45f);

    // 9. Central Magnetic Head Block with Dynamic Glowing Flux Core
    drawHeadBlock(g, headCenterX, headCenterY, headW, headH, currentFlux);
}

void TapeTransportComponent::drawBadassReel(juce::Graphics& g, float cx, float cy, float diameter, float angle, float tapePackFraction)
{
    float radius = diameter * 0.5f;

    // 1. Deep Cast Shadow under entire reel
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillEllipse(cx - radius - 1.0f, cy - radius + 3.0f, diameter + 2.0f, diameter + 2.0f);

    // 2. Wound Dark Ferric Oxide Tape Pack beneath the metal flanges
    float maxTapeRadius = radius - 3.0f;
    float minTapeRadius = radius * 0.38f;
    float tapeRadius = minTapeRadius + (maxTapeRadius - minTapeRadius) * tapePackFraction;

    juce::ColourGradient tapePackGrad(juce::Colour::fromRGB(54, 32, 16), cx - tapeRadius * 0.4f, cy - tapeRadius * 0.4f,
                                      juce::Colour::fromRGB(22, 11, 5), cx + tapeRadius * 0.6f, cy + tapeRadius * 0.6f, true);
    g.setGradientFill(tapePackGrad);
    g.fillEllipse(cx - tapeRadius, cy - tapeRadius, tapeRadius * 2.0f, tapeRadius * 2.0f);

    // Subtle concentric tape winding layers
    g.setColour(juce::Colour::fromRGB(72, 44, 22).withAlpha(0.35f));
    for (float r = minTapeRadius + 3.0f; r < tapeRadius; r += 2.5f)
    {
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.7f);
    }

    // 3. Spun Aluminum Reel Flange (CNC Anodized Brushed Gunmetal/Mauve)
    juce::ColourGradient flangeGrad(juce::Colour::fromRGB(75, 54, 98), cx - radius * 0.6f, cy - radius * 0.6f,
                                    juce::Colour::fromRGB(24, 15, 34), cx + radius * 0.8f, cy + radius * 0.8f, true);
    g.setGradientFill(flangeGrad);
    g.fillEllipse(cx - radius, cy - radius, diameter, diameter);

    // Outer Rim Specular Bevel
    juce::ColourGradient rimBevel(juce::Colour::fromRGB(190, 165, 218).withAlpha(0.65f), cx - radius, cy - radius,
                                  juce::Colour::fromRGB(45, 28, 62), cx + radius, cy + radius, false);
    g.setGradientFill(rimBevel);
    g.drawEllipse(cx - radius, cy - radius, diameter, diameter, 1.2f);

    // Inner lathe groove ring
    g.setColour(juce::Colour::fromRGB(130, 95, 165).withAlpha(0.25f));
    g.drawEllipse(cx - radius + 3.0f, cy - radius + 3.0f, diameter - 6.0f, diameter - 6.0f, 0.8f);

    // 4. Rotating Details (Iconic 3 Teardrop Cutouts & NAB Hub)
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(angle, cx, cy));

    // Three aerodynamic teardrop cutouts (Studer/Ampex style)
    for (int i = 0; i < 3; ++i)
    {
        float spokeAngle = i * (juce::MathConstants<float>::twoPi / 3.0f);

        juce::Path cutout;
        float rInner = radius * 0.40f;
        float rOuter = radius * 0.84f;
        float spanAngle = 0.55f; // Radian span of cutout

        cutout.addCentredArc(cx, cy, rInner, rInner, spokeAngle, -spanAngle * 0.5f, spanAngle * 0.5f, true);
        cutout.addCentredArc(cx, cy, rOuter, rOuter, spokeAngle, spanAngle * 0.45f, -spanAngle * 0.45f, false);
        cutout.closeSubPath();

        // Inner shadow revealing deep tape pack beneath
        g.setColour(juce::Colour::fromRGB(12, 6, 4));
        g.fillPath(cutout);

        // Tape oxide texture visible through cutout
        g.setColour(juce::Colour::fromRGB(48, 28, 14).withAlpha(0.6f));
        g.fillPath(cutout);

        // Precision beveled cutout border
        g.setColour(juce::Colour::fromRGB(150, 120, 185).withAlpha(0.70f));
        g.strokePath(cutout, juce::PathStrokeType(0.85f));
    }

    // Three CNC pilot index holes
    float indexDist = radius * 0.32f;
    for (int i = 0; i < 3; ++i)
    {
        float holeAngle = (i + 0.5f) * (juce::MathConstants<float>::twoPi / 3.0f);
        float hx = cx + indexDist * std::sin(holeAngle);
        float hy = cy - indexDist * std::cos(holeAngle);

        g.setColour(juce::Colour::fromRGB(12, 7, 18));
        g.fillEllipse(hx - 2.0f, hy - 2.0f, 4.0f, 4.0f);
        g.setColour(juce::Colour::fromRGB(160, 130, 195).withAlpha(0.8f));
        g.drawEllipse(hx - 2.0f, hy - 2.0f, 4.0f, 4.0f, 0.6f);
    }

    // 5. High-Contrast NAB Master Hub
    float hubRadius = 9.0f;
    // Hub base shadow
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.fillEllipse(cx - hubRadius - 0.5f, cy - hubRadius + 1.0f, hubRadius * 2.0f + 1.0f, hubRadius * 2.0f + 1.0f);

    // Stepped aluminum hub collar
    juce::ColourGradient hubGrad(juce::Colour::fromRGB(220, 205, 235), cx - hubRadius * 0.4f, cy - hubRadius * 0.4f,
                                 juce::Colour::fromRGB(65, 45, 85), cx + hubRadius * 0.7f, cy + hubRadius * 0.7f, true);
    g.setGradientFill(hubGrad);
    g.fillEllipse(cx - hubRadius, cy - hubRadius, hubRadius * 2.0f, hubRadius * 2.0f);
    g.setColour(juce::Colour::fromRGB(40, 24, 55));
    g.drawEllipse(cx - hubRadius, cy - hubRadius, hubRadius * 2.0f, hubRadius * 2.0f, 1.0f);

    // Three NAB hub locking dogs / splines
    for (int i = 0; i < 3; ++i)
    {
        float dogAngle = i * (juce::MathConstants<float>::twoPi / 3.0f);
        float dx = cx + (hubRadius - 2.5f) * std::sin(dogAngle);
        float dy = cy - (hubRadius - 2.5f) * std::cos(dogAngle);

        g.setColour(juce::Colour::fromRGB(25, 15, 35));
        g.fillRoundedRectangle(dx - 1.5f, dy - 1.5f, 3.0f, 3.0f, 0.8f);
        g.setColour(juce::Colour::fromRGB(220, 200, 240));
        g.drawRoundedRectangle(dx - 1.5f, dy - 1.5f, 3.0f, 3.0f, 0.8f, 0.5f);
    }

    // Center Spindle Pin (Mirror Stainless Steel)
    float pinRadius = 3.5f;
    juce::ColourGradient pinGrad(juce::Colours::white, cx - 1.0f, cy - 1.0f,
                                 juce::Colour::fromRGB(80, 80, 95), cx + 2.0f, cy + 2.0f, true);
    g.setGradientFill(pinGrad);
    g.fillEllipse(cx - pinRadius, cy - pinRadius, pinRadius * 2.0f, pinRadius * 2.0f);
    g.setColour(juce::Colour::fromRGB(20, 20, 25));
    g.drawEllipse(cx - pinRadius, cy - pinRadius, pinRadius * 2.0f, pinRadius * 2.0f, 0.8f);

    g.restoreState();
}

void TapeTransportComponent::drawHeadBlock(juce::Graphics& g, float cx, float cy, float w, float h, float flux)
{
    float blockX = cx - w * 0.5f;
    float blockY = cy - h * 0.5f;

    // 1. Deep Inset Drop Shadow
    g.setColour(juce::Colours::black.withAlpha(0.90f));
    g.fillRoundedRectangle(blockX, blockY + 2.0f, w, h, 3.0f);

    // 2. Shielded Permalloy Head Plate
    juce::ColourGradient headGrad(juce::Colour::fromRGB(62, 42, 85), blockX, blockY,
                                  juce::Colour::fromRGB(24, 14, 38), blockX, blockY + h, false);
    g.setGradientFill(headGrad);
    g.fillRoundedRectangle(blockX, blockY, w, h, 3.0f);

    // Plate metallic border
    g.setColour(juce::Colour::fromRGB(115, 80, 148));
    g.drawRoundedRectangle(blockX, blockY, w, h, 3.0f, 1.0f);

    // Top highlight rim
    g.setColour(juce::Colour::fromRGB(210, 185, 235).withAlpha(0.20f));
    g.drawLine(blockX + 4.0f, blockY + 1.0f, blockX + w - 4.0f, blockY + 1.0f, 1.0f);

    // 4 Corner micro-screws
    MauveLookAndFeel::drawChassisScrew(g, blockX + 4.0f, blockY + 4.0f, 5.0f);
    MauveLookAndFeel::drawChassisScrew(g, blockX + w - 4.0f, blockY + 4.0f, 5.0f);
    MauveLookAndFeel::drawChassisScrew(g, blockX + 4.0f, blockY + h - 4.0f, 5.0f);
    MauveLookAndFeel::drawChassisScrew(g, blockX + w - 4.0f, blockY + h - 4.0f, 5.0f);

    // 3. Dynamic "FLUX CORE" Silkscreen with Audio Intensity Glow
    float labelFlux = juce::jlimit(0.0f, 1.0f, flux * 1.2f);
    juce::Colour labelColor = MauveTheme::amber.interpolatedWith(juce::Colour::fromRGB(255, 245, 210), labelFlux);

    g.setColour(labelColor);
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.drawText("FLUX CORE", juce::Rectangle<float>(blockX, blockY + 2.0f, w, 10.0f), juce::Justification::centred, false);

    // 4. Dual Shielded Magnetic Head Assemblies (Record Head & Reproduce Head)
    float headGapW = 18.0f;
    float headGapH = 17.0f;
    float h1X = blockX + 6.0f;
    float h2X = blockX + w - headGapW - 6.0f;
    float hY  = blockY + 12.0f;

    auto drawMagneticHead = [&](float x, bool /*isRepro*/)
    {
        // MU-metal head shield cavity
        g.setColour(juce::Colour::fromRGB(9, 5, 14));
        g.fillRoundedRectangle(x, hY, headGapW, headGapH, 2.0f);
        g.setColour(juce::Colour::fromRGB(80, 53, 108));
        g.drawRoundedRectangle(x, hY, headGapW, headGapH, 2.0f, 0.8f);

        // Core pole pieces (Left and Right permalloy blocks)
        float poleW = 6.0f;
        float poleH = headGapH - 4.0f;
        float poleY = hY + 2.0f;

        // Left pole
        juce::ColourGradient p1(juce::Colour::fromRGB(55, 40, 72), x + 1.5f, poleY,
                                juce::Colour::fromRGB(25, 16, 36), x + 1.5f + poleW, poleY + poleH, false);
        g.setGradientFill(p1);
        g.fillRoundedRectangle(x + 2.0f, poleY, poleW, poleH, 1.0f);

        // Right pole
        juce::ColourGradient p2(juce::Colour::fromRGB(55, 40, 72), x + headGapW - poleW - 2.0f, poleY,
                                juce::Colour::fromRGB(25, 16, 36), x + headGapW - 2.0f, poleY + poleH, false);
        g.setGradientFill(p2);
        g.fillRoundedRectangle(x + headGapW - poleW - 2.0f, poleY, poleW, poleH, 1.0f);

        // Central Magnetic Gap (Where the magnetic flux couples to tape)
        float gapCenterX = x + headGapW * 0.5f;
        float gapCenterY = hY + headGapH * 0.5f;
        float gapH = headGapH - 3.0f;

        // --- BLING: DYNAMIC FLUX CORE GLOW UP WITH INTENSITY ---
        // 1. Broad soft amber bloom radiating outward from gap
        float bRadius = 6.0f + 14.0f * flux;
        float bAlpha = 0.25f + 0.65f * juce::jlimit(0.0f, 1.0f, flux);
        juce::Colour outerAmber = juce::Colour::fromRGB(255, 110, 0).withAlpha(bAlpha);
        juce::ColourGradient gapBloom(outerAmber, gapCenterX, gapCenterY,
                                      juce::Colours::transparentBlack, gapCenterX, gapCenterY + bRadius, true);
        g.setGradientFill(gapBloom);
        g.fillEllipse(gapCenterX - bRadius, gapCenterY - bRadius, bRadius * 2.0f, bRadius * 2.0f);

        // 2. High-intensity incandescent filament gap
        float gapWidth = 2.0f + 1.5f * juce::jlimit(0.0f, 1.0f, flux);
        g.setColour(juce::Colour::fromRGB(255, 150, 0).withAlpha(0.85f));
        g.fillRoundedRectangle(gapCenterX - gapWidth * 0.5f, gapCenterY - gapH * 0.5f, gapWidth, gapH, 1.0f);

        // 3. Golden yellow inner plasma core
        float yellowWidth = 1.4f + 0.8f * juce::jlimit(0.0f, 1.0f, flux);
        g.setColour(juce::Colour::fromRGB(255, 230, 90).withAlpha(0.90f));
        g.fillRoundedRectangle(gapCenterX - yellowWidth * 0.5f, gapCenterY - gapH * 0.5f, yellowWidth, gapH, 0.7f);

        // 4. White-hot flux saturation filament
        float whiteAlpha = juce::jlimit(0.35f, 1.0f, 0.20f + 0.80f * flux);
        g.setColour(juce::Colours::white.withAlpha(whiteAlpha));
        g.fillRect(gapCenterX - 0.6f, gapCenterY - gapH * 0.5f, 1.2f, gapH);
    };

    drawMagneticHead(h1X, false); // Record Head
    drawMagneticHead(h2X, true);  // Reproduce Head

    // 5. Center Flux Meter Jewel LED between heads
    float jewelX = cx;
    float jewelY = hY + headGapH * 0.5f;
    float jewelAlpha = juce::jlimit(0.30f, 1.0f, 0.25f + 0.75f * flux);
    g.setColour(MauveTheme::amber.withAlpha(jewelAlpha * 0.5f));
    g.fillEllipse(jewelX - 3.5f, jewelY - 3.5f, 7.0f, 7.0f);
    g.setColour(MauveTheme::amberBright.withAlpha(jewelAlpha));
    g.fillEllipse(jewelX - 1.5f, jewelY - 1.5f, 3.0f, 3.0f);

    // 6. Sub-label: HYSTERESIS
    g.setColour(MauveTheme::silkscreen);
    g.setFont(juce::FontOptions(6.5f, juce::Font::bold));
    g.drawText("HYSTERESIS", juce::Rectangle<float>(blockX, blockY + h - 10.0f, w, 8.0f), juce::Justification::centred, false);
}
