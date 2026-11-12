#include "MauveLookAndFeel.h"
#include <cmath>

MauveLookAndFeel::MauveLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(12, 8, 20));
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(68, 47, 89));
    setColour(juce::ComboBox::textColourId, MauveTheme::amberBright);
    setColour(juce::ComboBox::arrowColourId, MauveTheme::amber);

    setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(14, 9, 23));
    setColour(juce::PopupMenu::textColourId, MauveTheme::amberBright);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB(58, 37, 80));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void MauveLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto maxDiameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 2.0f;
    auto centre = bounds.getCentre();

    // Reserve outer 5.5px radius for the glowing phosphor scale ring & ticks
    float arcRadius = maxDiameter * 0.5f - 2.0f;
    float diameter  = maxDiameter - 12.0f;
    float radius    = diameter * 0.5f;

    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    bool isFluted = slider.getProperties().contains("fluted");
    bool isBipolar = (slider.getMinimum() < -0.01 && slider.getMaximum() > 0.01);

    // 0. Outer Precision Tick Marks (11 ticks from rotaryStartAngle to rotaryEndAngle)
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(numTicks - 1);
        float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        bool isMajor = (i == 0 || i == 5 || i == 10);

        float rIn  = arcRadius + 1.2f;
        float rOut = arcRadius + (isMajor ? 3.5f : 2.3f);
        float sinA = std::sin(tickAngle);
        float cosA = -std::cos(tickAngle);

        bool isLit = isBipolar
            ? ((tickAngle >= std::min(0.0f, angle) - 0.02f) && (tickAngle <= std::max(0.0f, angle) + 0.02f))
            : (t <= sliderPosProportional + 0.01f);

        g.setColour(isLit ? MauveTheme::amber.withAlpha(isMajor ? 0.90f : 0.65f)
                          : MauveTheme::silkscreenMuted.withAlpha(isMajor ? 0.45f : 0.22f));
        g.drawLine(centre.x + rIn * sinA, centre.y + rIn * cosA,
                   centre.x + rOut * sinA, centre.y + rOut * cosA,
                   isMajor ? 1.3f : 0.9f);
    }

    // 0.5 Recessed LED Arc Track & Active Phosphor Value Arc
    {
        juce::Path bgArc;
        bgArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour::fromRGB(16, 10, 25));
        g.strokePath(bgArc, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float startA = isBipolar ? 0.0f : rotaryStartAngle;
        float endA   = angle;
        if (std::abs(endA - startA) > 0.015f)
        {
            juce::Path valArc;
            valArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 std::min(startA, endA), std::max(startA, endA), true);

            // Outer amber phosphor bloom
            g.setColour(MauveTheme::amber.withAlpha(0.28f));
            g.strokePath(valArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Core illuminated arc
            g.setColour(MauveTheme::amber);
            g.strokePath(valArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // 1. Realistic Deep Drop Shadow
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillEllipse(centre.x - radius, centre.y - radius + 3.5f, diameter, diameter);

    // 2. Outer Dark Bakelite Collar
    g.setColour(juce::Colour::fromRGB(10, 6, 16));
    g.fillEllipse(centre.x - radius, centre.y - radius, diameter, diameter);

    // 2.5 Fluted CNC Grip Knurling Notches (for Deck 3 Tape Dials)
    if (isFluted)
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angle, centre.x, centre.y));
        const int numFlutes = 18;
        g.setColour(juce::Colour::fromRGB(145, 110, 180).withAlpha(0.25f));
        for (int i = 0; i < numFlutes; ++i)
        {
            float a = i * (juce::MathConstants<float>::twoPi / static_cast<float>(numFlutes));
            float fx = centre.x + (radius - 0.8f) * std::sin(a);
            float fy = centre.y - (radius - 0.8f) * std::cos(a);
            g.fillEllipse(fx - 1.2f, fy - 1.2f, 2.4f, 2.4f);
        }
        g.restoreState();
    }

    // 3. Knob Face with 3D Radial Gradient
    float innerRadius = radius - 1.8f;
    float innerDiameter = innerRadius * 2.0f;

    juce::Colour lightCol = isFluted ? juce::Colour::fromRGB(92, 65, 120)
                                     : juce::Colour::fromRGB(70, 48, 94);
    juce::Colour darkCol  = isFluted ? juce::Colour::fromRGB(24, 14, 34)
                                     : juce::Colour::fromRGB(18, 10, 26);

    float lightX = centre.x - innerRadius * 0.32f;
    float lightY = centre.y - innerRadius * 0.32f;

    juce::ColourGradient faceGrad(lightCol, lightX, lightY,
                                  darkCol, centre.x + innerRadius * 0.65f, centre.y + innerRadius * 0.65f,
                                  true);
    g.setGradientFill(faceGrad);
    g.fillEllipse(centre.x - innerRadius, centre.y - innerRadius, innerDiameter, innerDiameter);

    // 3.5 Inner Machined Lathe Cap Ring
    float capRadius = innerRadius * 0.56f;
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawEllipse(centre.x - capRadius, centre.y - capRadius + 0.6f, capRadius * 2.0f, capRadius * 2.0f, 0.9f);
    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.14f));
    g.drawEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 0.7f);

    // 4. Slim Inset Top-Left Bevel Highlight
    g.setColour(juce::Colour::fromRGB(210, 188, 232).withAlpha(0.34f));
    juce::Path bevel;
    bevel.addCentredArc(centre.x, centre.y, innerRadius - 0.5f, innerRadius - 0.5f,
                        0.0f, juce::degreesToRadians(-135.0f), juce::degreesToRadians(45.0f), true);
    g.strokePath(bevel, juce::PathStrokeType(1.1f));

    // Bottom-right inner shadow
    g.setColour(juce::Colours::black.withAlpha(0.70f));
    juce::Path botBevel;
    botBevel.addCentredArc(centre.x, centre.y, innerRadius - 0.5f, innerRadius - 0.5f,
                           0.0f, juce::degreesToRadians(45.0f), juce::degreesToRadians(225.0f), true);
    g.strokePath(botBevel, juce::PathStrokeType(1.1f));

    // 5. Slim Precision Amber Indicator Notch Line
    bool isLarge = slider.getProperties().contains("large") || diameter > 72.0f;
    float pointerThickness = isLarge ? 2.0f : 1.7f;
    float pointerLength = innerRadius * 0.58f;
    float pointerTopY = centre.y - innerRadius + 2.2f;

    juce::Path pointerPath;
    pointerPath.addRoundedRectangle(centre.x - pointerThickness * 0.5f,
                                    pointerTopY,
                                    pointerThickness,
                                    pointerLength,
                                    0.9f);

    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(angle, centre.x, centre.y));

    // Amber Glow
    g.setColour(MauveTheme::amber.withAlpha(0.55f));
    g.strokePath(pointerPath, juce::PathStrokeType(3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Core Indicator Line (Vivid Amber)
    g.setColour(MauveTheme::amber);
    g.fillPath(pointerPath);

    // Tip highlight
    g.setColour(MauveTheme::amberBright);
    g.fillRect(centre.x - 0.45f, pointerTopY + 0.5f, 0.9f, pointerLength - 1.5f);

    g.restoreState();
}

void MauveLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& /*backgroundColour*/,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

    if (button.getButtonText().contains("IPS"))
    {
        // Mechanical Tape Odometer Counter Box
        g.setColour(juce::Colour::fromRGB(10, 6, 2));
        g.fillRoundedRectangle(bounds, 2.5f);

        juce::ColourGradient insetShadow(juce::Colours::black.withAlpha(0.85f), 0.0f, bounds.getY(),
                                         juce::Colours::transparentBlack, 0.0f, bounds.getY() + 5.0f, false);
        g.setGradientFill(insetShadow);
        g.fillRoundedRectangle(bounds, 2.5f);

        g.setColour(shouldDrawButtonAsHighlighted ? MauveTheme::amberBright : MauveTheme::amberBorder);
        g.drawRoundedRectangle(bounds, 2.5f, 1.0f);
        return;
    }

    bool isActive = button.getToggleState();

    juce::Colour bg = isActive ? MauveTheme::btnActiveBg
                               : (shouldDrawButtonAsHighlighted ? juce::Colour::fromRGB(35, 24, 48) : MauveTheme::btnBg);
    if (shouldDrawButtonAsDown)
        bg = bg.darker(0.2f);

    // Subtle drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 3.0f);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 3.0f);

    juce::Colour borderCol = isActive ? MauveTheme::amber
                                      : (shouldDrawButtonAsHighlighted ? juce::Colour::fromRGB(115, 82, 148) : MauveTheme::btnBorder);
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, 3.0f, isActive ? 1.4f : 1.0f);

    if (isActive)
    {
        g.setColour(MauveTheme::amber.withAlpha(0.30f));
        g.drawRoundedRectangle(bounds.expanded(1.0f), 3.5f, 1.4f);
    }
}

void MauveLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool shouldDrawButtonAsHighlighted,
                                      bool /*shouldDrawButtonAsDown*/)
{
    if (button.getButtonText().contains("IPS"))
    {
        g.setColour(MauveTheme::amberBright);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
        return;
    }

    if (button.getButtonText() == "BYPASS")
    {
        bool isActive = button.getToggleState();
        g.setColour(isActive ? MauveTheme::amberBright : (shouldDrawButtonAsHighlighted ? MauveTheme::textBright : MauveTheme::silkscreen));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("BYPASS", button.getLocalBounds(), juce::Justification::centred, false);
        return;
    }

    bool isActive = button.getToggleState();
    g.setColour(isActive ? MauveTheme::amberBright : (shouldDrawButtonAsHighlighted ? MauveTheme::textBright : MauveTheme::silkscreen));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void MauveLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int buttonX, int buttonY, int buttonW, int buttonH,
                                    juce::ComboBox& /*box*/)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 3.0f);

    g.setColour(findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 3.0f);

    juce::ColourGradient insetGrad(juce::Colours::black.withAlpha(0.45f), 0.0f, bounds.getY(),
                                   juce::Colours::transparentBlack, 0.0f, bounds.getY() + 4.5f, false);
    g.setGradientFill(insetGrad);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // Arrow
    juce::Path arrow;
    auto arrowBox = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat();
    auto arrowCenter = arrowBox.getCentre();
    arrow.addTriangle(arrowCenter.x - 3.5f, arrowCenter.y - 1.5f,
                      arrowCenter.x + 3.5f, arrowCenter.y - 1.5f,
                      arrowCenter.x, arrowCenter.y + 2.5f);

    g.setColour(findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

juce::Font MauveLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::FontOptions(11.0f, juce::Font::bold);
}

juce::PopupMenu::Options MauveLookAndFeel::getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label&)
{
    return juce::PopupMenu::Options()
        .withTargetComponent(&box)
        .withTargetScreenArea(box.getScreenBounds())
        .withMinimumWidth(box.getWidth())
        .withMaximumNumColumns(1)
        .withStandardItemHeight(24);
}

void MauveLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& sectionName)
{
    g.setColour(juce::Colour::fromRGB(52, 36, 68));
    g.drawHorizontalLine(area.getY() + 1, (float)area.getX() + 6.0f, (float)area.getRight() - 6.0f);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(MauveTheme::silkscreen.withAlpha(0.70f));
    g.drawText(sectionName, area.reduced(10, 0), juce::Justification::centredLeft, true);
}

juce::Font MauveLookAndFeel::getPopupMenuFont()
{
    return juce::FontOptions(11.5f, juce::Font::bold);
}

void MauveLookAndFeel::drawChassisScrew(juce::Graphics& g, float cx, float cy, float diameter)
{
    float radius = diameter * 0.5f;

    g.setColour(juce::Colour::fromRGB(10, 6, 16));
    g.fillEllipse(cx - radius, cy - radius, diameter, diameter);

    juce::ColourGradient grad(juce::Colour::fromRGB(88, 62, 114), cx - radius * 0.35f, cy - radius * 0.35f,
                              juce::Colour::fromRGB(19, 12, 28), cx + radius * 0.6f, cy + radius * 0.6f,
                              true);
    g.setGradientFill(grad);
    g.fillEllipse(cx - radius + 0.8f, cy - radius + 0.8f, diameter - 1.6f, diameter - 1.6f);

    g.setColour(juce::Colour::fromRGB(200, 176, 221).withAlpha(0.28f));
    g.drawEllipse(cx - radius + 1.0f, cy - radius + 1.0f, diameter - 2.0f, diameter - 2.0f, 0.8f);

    // 42-degree slotted screw head line
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(juce::degreesToRadians(42.0f), cx, cy));
    g.setColour(juce::Colour::fromRGB(7, 4, 10));
    g.fillRect(cx - radius + 2.2f, cy - 0.8f, diameter - 4.4f, 1.6f);
    g.restoreState();
}
