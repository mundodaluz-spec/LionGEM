#pragma once
#include "KingLionTheme.h"

namespace kinglion::gui
{

/**
 * LookAndFeel Boutique Moderno para o KingLion Box 1.0 (WYATT Sounds).
 * Desenha Knobs Usinados 3D com anel de progresso ambar sutil, Chaves Toggle
 * Cromadas 3D e Seletor de Presets estilo hardware de luxo.
 */
class KingLionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    KingLionLookAndFeel()
    {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff161512));
        setColour(juce::ComboBox::textColourId,       Palette::goldLight);
        setColour(juce::ComboBox::outlineColourId,    Palette::goldDark);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff181714));
        setColour(juce::PopupMenu::textColourId,       Palette::textCream);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::goldDark.withAlpha(0.65f));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                   static_cast<float>(width), static_cast<float>(height));
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.39f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Anel externo e arco luminoso ambar
        const float arcRadius = radius + 6.0f;
        juce::Path trackArc;
        trackArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.strokePath(trackArc, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            juce::Path valueArc;
            const bool bipolar = (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0);
            const float zeroNorm = bipolar
                ? static_cast<float>(-slider.getMinimum() / (slider.getMaximum() - slider.getMinimum()))
                : 0.0f;
            const float startValAngle = rotaryStartAngle + zeroNorm * (rotaryEndAngle - rotaryStartAngle);

            valueArc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, startValAngle, angle, true);
            g.setColour(Palette::accentAmber.withAlpha(0.80f));
            g.strokePath(valueArc, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Marcadores radiais (ticks)
        const int customTicks = slider.getProperties().getWithDefault("numTicks", 11);
        for (int i = 0; i < customTicks; ++i)
        {
            const float t = (customTicks > 1) ? static_cast<float>(i) / static_cast<float>(customTicks - 1) : 0.5f;
            const float ta = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle) - juce::MathConstants<float>::halfPi;
            const bool major = (i == 0 || i == customTicks - 1 || i == customTicks / 2);
            const float r1 = radius + 8.5f;
            const float r2 = radius + (major ? 12.0f : 10.4f);
            g.setColour(major ? Palette::textGold.withAlpha(0.82f) : Palette::textMuted.withAlpha(0.42f));
            g.drawLine(cx + std::cos(ta) * r1, cy + std::sin(ta) * r1,
                       cx + std::cos(ta) * r2, cy + std::sin(ta) * r2, 1.2f);
        }

        // Sombra profunda da saia do knob
        g.setColour(juce::Colours::black.withAlpha(0.72f));
        g.fillEllipse(cx - radius - 1.5f, cy - radius + 3.2f, (radius + 1.5f) * 2.0f, (radius + 1.5f) * 2.0f);

        // Saia estriada externa (knurled anodized skirt)
        juce::ColourGradient skirtGrad(juce::Colour(0xff3a3d45), cx - radius, cy - radius,
                                       juce::Colour(0xff101114), cx + radius, cy + radius, false);
        g.setGradientFill(skirtGrad);
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(Palette::goldDark.withAlpha(0.50f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.1f);

        // Topo escovado do knob
        const float capR = radius * 0.78f;
        juce::ColourGradient capGrad(juce::Colour(0xff2e3138), cx, cy - capR,
                                     juce::Colour(0xff15171b), cx, cy + capR, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.13f));
        g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 0.9f);

        // Ponteiro indicador luminoso
        const float pointerAngle = angle - juce::MathConstants<float>::halfPi;
        const float pInner = capR * 0.24f;
        const float pOuter = radius * 0.92f;

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(cx + std::cos(pointerAngle) * pInner + 0.8f,
                   cy + std::sin(pointerAngle) * pInner + 0.8f,
                   cx + std::cos(pointerAngle) * pOuter + 0.8f,
                   cy + std::sin(pointerAngle) * pOuter + 0.8f, 3.2f);

        g.setColour(Palette::textCream);
        g.drawLine(cx + std::cos(pointerAngle) * pInner,
                   cy + std::sin(pointerAngle) * pInner,
                   cx + std::cos(pointerAngle) * pOuter,
                   cy + std::sin(pointerAngle) * pOuter, 2.3f);
    }

    /**
     * Chave Alavanca Cromada 3D de 2 Posicoes (ToggleButton)
     */
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        const auto b = button.getLocalBounds().toFloat();
        const float cx = b.getCentreX();
        const float cy = b.getCentreY();
        const bool invertLever = button.getProperties().getWithDefault("invertLever", false);
        const bool leverUp = invertLever ? !button.getToggleState() : button.getToggleState();

        const float bushR = 9.2f;
        g.setColour(juce::Colours::black.withAlpha(0.70f));
        g.fillEllipse(cx - bushR, cy - bushR + 2.0f, bushR * 2.0f, bushR * 2.0f);

        juce::ColourGradient bushGrad(juce::Colour(0xffe2e5ea), cx - bushR, cy - bushR,
                                      juce::Colour(0xff5b6069), cx + bushR, cy + bushR, false);
        g.setGradientFill(bushGrad);
        g.fillEllipse(cx - bushR, cy - bushR, bushR * 2.0f, bushR * 2.0f);

        g.setColour(juce::Colour(0xff141619));
        g.fillEllipse(cx - 4.6f, cy - 4.6f, 9.2f, 9.2f);

        const float tipY = leverUp ? (cy - 11.0f) : (cy + 11.0f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawLine(cx, cy + 1.5f, cx + 1.2f, tipY + 2.0f, 6.2f);

        juce::ColourGradient shaftGrad(juce::Colours::white, cx - 3.0f, cy,
                                       juce::Colour(0xff7d838e), cx + 3.0f, tipY + 0.1f, false);
        g.setGradientFill(shaftGrad);
        g.drawLine(cx, cy, cx, tipY, 5.2f);

        const float tipR = 4.5f;
        juce::ColourGradient ballGrad(juce::Colours::white, cx - 2.0f, tipY - 2.0f,
                                      juce::Colour(0xff6d737d), cx + 2.5f, tipY + 2.5f, true);
        g.setGradientFill(ballGrad);
        g.fillEllipse(cx - tipR, tipY - tipR, tipR * 2.0f, tipR * 2.0f);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                      juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        g.setColour(juce::Colour(0xff171510));
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(Palette::goldDark.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.reduced(0.6f), 5.0f, 1.2f);

        juce::Path arrow;
        const float ax = static_cast<float>(width) - 15.0f;
        const float ay = static_cast<float>(height) * 0.5f;
        arrow.addTriangle(ax - 4.5f, ay - 2.5f, ax + 4.5f, ay - 2.5f, ax, ay + 3.0f);
        g.setColour(box.isEnabled() ? Palette::goldLight : Palette::textMuted);
        g.fillPath(arrow);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return makeFont(12.5f, true, false);
    }
};

/**
 * Chave Alavanca Cromada de 3 Posicoes Verticais (CORTE 120Hz/80Hz/FLAT e DE-ESSER/DESL./LIMITADOR).
 */
class ThreeWayToggleSwitch final : public juce::Component
{
public:
    explicit ThreeWayToggleSwitch(juce::RangedAudioParameter* paramToControl, bool invertOrder = false)
        : parameter(paramToControl), inverted(invertOrder)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float cx = b.getCentreX();
        const float cy = b.getCentreY();

        int idx = 0;
        if (parameter != nullptr)
            idx = static_cast<int>(std::round(parameter->convertFrom0to1(parameter->getValue())));

        int visualPos = 0;
        if (inverted)
            visualPos = (idx == 2) ? -1 : (idx == 1 ? 0 : 1);
        else
            visualPos = (idx == 0) ? -1 : (idx == 1 ? 0 : 1);

        const float bushR = 9.2f;
        g.setColour(juce::Colours::black.withAlpha(0.70f));
        g.fillEllipse(cx - bushR, cy - bushR + 2.0f, bushR * 2.0f, bushR * 2.0f);

        juce::ColourGradient bushGrad(juce::Colour(0xffe2e5ea), cx - bushR, cy - bushR,
                                      juce::Colour(0xff5b6069), cx + bushR, cy + bushR, false);
        g.setGradientFill(bushGrad);
        g.fillEllipse(cx - bushR, cy - bushR, bushR * 2.0f, bushR * 2.0f);

        g.setColour(juce::Colour(0xff141619));
        g.fillEllipse(cx - 4.6f, cy - 4.6f, 9.2f, 9.2f);

        const float tipY = cy + static_cast<float>(visualPos) * 11.0f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawLine(cx, cy + 1.5f, cx + 1.0f, tipY + 1.8f, 6.0f);

        juce::ColourGradient shaftGrad(juce::Colours::white, cx - 3.0f, cy,
                                       juce::Colour(0xff7d838e), cx + 3.0f, tipY + 0.1f, false);
        g.setGradientFill(shaftGrad);
        g.drawLine(cx, cy, cx, tipY, 5.2f);

        const float tipR = 4.5f;
        juce::ColourGradient ballGrad(juce::Colours::white, cx - 2.0f, tipY - 2.0f,
                                      juce::Colour(0xff6d737d), cx + 2.5f, tipY + 2.5f, true);
        g.setGradientFill(ballGrad);
        g.fillEllipse(cx - tipR, tipY - tipR, tipR * 2.0f, tipR * 2.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (parameter == nullptr)
            return;

        const float relY = static_cast<float>(e.y) / std::max(1.0f, static_cast<float>(getHeight()));
        int visualBand = (relY < 0.36f) ? 0 : (relY < 0.66f ? 1 : 2);
        int targetChoice = inverted ? (2 - visualBand) : visualBand;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(targetChoice)));
        parameter->endChangeGesture();
        repaint();
    }

private:
    juce::RangedAudioParameter* parameter = nullptr;
    bool inverted = false;
};

} // namespace kinglion::gui
