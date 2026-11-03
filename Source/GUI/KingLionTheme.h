#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace kinglion::gui
{

struct Palette
{
    static inline const juce::Colour goldLight      { 0xffe8cb82 };
    static inline const juce::Colour goldMid        { 0xffcfa14b };
    static inline const juce::Colour goldDark       { 0xffa67926 };
    static inline const juce::Colour goldDeepBorder { 0xff6e4f16 };

    static inline const juce::Colour panelTop       { 0xff1a1c20 };
    static inline const juce::Colour panelBottom    { 0xff0d0e11 };
    static inline const juce::Colour panelTrim      { 0xff8b6d35 };

    static inline const juce::Colour textGold       { 0xffe5c67b };
    static inline const juce::Colour textCream      { 0xffe9e4d8 };
    static inline const juce::Colour textMuted      { 0xff9a968c };
    static inline const juce::Colour accentAmber    { 0xffffad33 };
    static inline const juce::Colour accentRed      { 0xffd93226 };
};

inline juce::Font makeFont(float heightPx, bool bold = false, bool serif = false)
{
    auto opts = juce::FontOptions(heightPx)
                    .withStyleFlags(bold ? juce::Font::bold : juce::Font::plain);

    if (serif)
        opts = opts.withName("Georgia");

    return juce::Font(opts);
}

inline void drawMachinedScrew(juce::Graphics& g, float cx, float cy, float radius)
{
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillEllipse(cx - radius - 1.0f, cy - radius + 1.0f, radius * 2.0f + 2.0f, radius * 2.0f + 2.0f);

    juce::ColourGradient grad(juce::Colour(0xffd4d7dc), cx - radius * 0.5f, cy - radius * 0.5f,
                              juce::Colour(0xff5a5e66), cx + radius * 0.7f, cy + radius * 0.7f, true);
    g.setGradientFill(grad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(0xff2b2e33));
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.8f);

    g.setColour(juce::Colour(0xff232529));
    const float slot = radius * 0.58f;
    g.drawLine(cx - slot, cy, cx + slot, cy, 1.3f);
    g.drawLine(cx, cy - slot, cx, cy + slot, 1.3f);
}

inline void drawSubPanel(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
{
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 3.0f), 7.0f);

    juce::ColourGradient bg(Palette::panelTop, bounds.getX(), bounds.getY(),
                            Palette::panelBottom, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 7.0f);

    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.drawRoundedRectangle(bounds, 7.0f, 1.5f);

    g.setColour(Palette::panelTrim.withAlpha(0.55f));
    g.drawRoundedRectangle(bounds.reduced(1.5f), 6.0f, 1.0f);

    const float inset = 11.5f;
    drawMachinedScrew(g, bounds.getX() + inset,     bounds.getY() + inset,      4.4f);
    drawMachinedScrew(g, bounds.getRight() - inset, bounds.getY() + inset,      4.4f);
    drawMachinedScrew(g, bounds.getX() + inset,     bounds.getBottom() - inset, 4.4f);
    drawMachinedScrew(g, bounds.getRight() - inset, bounds.getBottom() - inset, 4.4f);

    if (title.isNotEmpty())
    {
        g.setFont(makeFont(12.0f, true, false));
        g.setColour(Palette::textGold);
        auto headerRect = juce::Rectangle<float>(bounds.getX() + 26.0f, bounds.getY() + 6.0f,
                                                 bounds.getWidth() - 52.0f, 18.0f);
        g.drawText(title, headerRect, juce::Justification::centred, false);

        g.setColour(Palette::panelTrim.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 26.0f),
                             bounds.getX() + 20.0f, bounds.getRight() - 20.0f);
    }
}

inline void drawGoldenLionMedallion(juce::Graphics& g, float cx, float cy, float radius)
{
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - radius - 1.5f, cy - radius + 2.5f, (radius + 1.5f) * 2.0f, (radius + 1.5f) * 2.0f);

    juce::ColourGradient rimGrad(juce::Colour(0xfff7df9c), cx - radius, cy - radius,
                                 juce::Colour(0xff8c6218), cx + radius, cy + radius, false);
    g.setGradientFill(rimGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    const float innerR = radius * 0.84f;
    juce::ColourGradient coreGrad(juce::Colour(0xff231d14), cx, cy - innerR,
                                  juce::Colour(0xff090806), cx, cy + innerR, false);
    g.setGradientFill(coreGrad);
    g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

    g.setColour(Palette::goldLight.withAlpha(0.7f));
    g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

    juce::Path mane;
    constexpr int numRays = 14;
    for (int i = 0; i < numRays; ++i)
    {
        const float a0 = juce::MathConstants<float>::twoPi * (static_cast<float>(i) / numRays) - 1.5708f;
        const float aMid = a0 + juce::MathConstants<float>::pi / numRays;
        const float a1 = a0 + juce::MathConstants<float>::twoPi / numRays;

        const float rIn = radius * 0.42f;
        const float rTip = (i % 2 == 0) ? radius * 0.74f : radius * 0.64f;

        mane.startNewSubPath(cx + std::cos(a0) * rIn, cy + std::sin(a0) * rIn);
        mane.quadraticTo(cx + std::cos(aMid) * rTip, cy + std::sin(aMid) * rTip,
                         cx + std::cos(a1) * rIn,    cy + std::sin(a1) * rIn);
        mane.closeSubPath();
    }

    juce::ColourGradient lionGrad(juce::Colour(0xffffe59e), cx, cy - radius * 0.65f,
                                  juce::Colour(0xffb88020), cx, cy + radius * 0.65f, false);
    g.setGradientFill(lionGrad);
    g.fillPath(mane);

    juce::Path face;
    const float s = radius * 0.44f;
    face.startNewSubPath(cx, cy - s * 0.95f);
    face.lineTo(cx + s * 0.72f, cy - s * 0.55f);
    face.lineTo(cx + s * 0.82f, cy + s * 0.15f);
    face.lineTo(cx + s * 0.45f, cy + s * 0.85f);
    face.lineTo(cx,             cy + s * 1.05f);
    face.lineTo(cx - s * 0.45f, cy + s * 0.85f);
    face.lineTo(cx - s * 0.82f, cy + s * 0.15f);
    face.lineTo(cx - s * 0.72f, cy - s * 0.55f);
    face.closeSubPath();

    g.setColour(juce::Colour(0xff1a1308));
    g.fillPath(face.createPathWithRoundedCorners(2.0f), juce::AffineTransform::scale(1.10f, 1.10f, cx, cy));

    g.setGradientFill(lionGrad);
    g.fillPath(face.createPathWithRoundedCorners(2.0f));

    g.setColour(juce::Colour(0xff231707));
    g.fillTriangle(cx - s * 0.48f, cy - s * 0.18f, cx - s * 0.14f, cy - s * 0.06f, cx - s * 0.36f, cy - s * 0.30f);
    g.fillTriangle(cx + s * 0.48f, cy - s * 0.18f, cx + s * 0.14f, cy - s * 0.06f, cx + s * 0.36f, cy - s * 0.30f);
    g.fillTriangle(cx - s * 0.20f, cy + s * 0.22f, cx + s * 0.20f, cy + s * 0.22f, cx, cy + s * 0.44f);
    g.drawLine(cx, cy + s * 0.44f, cx, cy + s * 0.68f, 1.4f);
    g.drawLine(cx - s * 0.24f, cy + s * 0.68f, cx + s * 0.24f, cy + s * 0.68f, 1.3f);
}

} // namespace kinglion::gui
