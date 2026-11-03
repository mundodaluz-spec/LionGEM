#pragma once
#include "KingLionTheme.h"

namespace kinglion::gui
{

/**
 * Medidor VU Iluminado Estilo Sifam Vintage-Moderno.
 * Possui retroiluminacao ambar-mel quente, zona vermelha +0..+3 VU, sombra realista
 * do ponteiro, reflexo curvo de vidro e badge digital de leitura instantanea.
 */
class VuMeterComponent final : public juce::Component
{
public:
    void setMeterState(float needleVuDb, int meterModeIndex, bool powerActive) noexcept
    {
        currentDb = juce::jlimit(-20.0f, 3.5f, needleVuDb);
        modeIdx = juce::jlimit(0, 4, meterModeIndex);
        isPowered = powerActive;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Moldura externa chanfrada chumbo/preta
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 2.5f), 11.0f);

        juce::ColourGradient bezelGrad(juce::Colour(0xff2c2f36), bounds.getX(), bounds.getY(),
                                       juce::Colour(0xff101215), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(bezelGrad);
        g.fillRoundedRectangle(bounds, 11.0f);

        g.setColour(Palette::goldDark.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.reduced(0.8f), 10.5f, 1.2f);

        const auto glass = bounds.reduced(8.0f);
        g.saveState();
        juce::Path clipPath;
        clipPath.addRoundedRectangle(glass, 7.0f);
        g.reduceClipRegion(clipPath);

        // Fundo iluminado ambar Sifam (escurece suavemente quando desligado)
        const juce::Colour centerGlow = isPowered ? juce::Colour(0xffffdc78) : juce::Colour(0xff6b5b3e);
        const juce::Colour edgeGlow   = isPowered ? juce::Colour(0xffdf8b20) : juce::Colour(0xff3b3222);
        juce::ColourGradient lampGrad(centerGlow, glass.getCentreX(), glass.getY() + glass.getHeight() * 0.55f,
                                      edgeGlow,   glass.getX(),       glass.getY(), true);
        g.setGradientFill(lampGrad);
        g.fillRect(glass);

        // Sombra interna da moldura sobre o mostrador
        g.setColour(juce::Colours::black.withAlpha(0.28f));
        g.drawRoundedRectangle(glass, 7.0f, 3.5f);

        // Geometria do arco da escala VU
        const float pivotX = glass.getCentreX();
        const float pivotY = glass.getBottom() + glass.getHeight() * 0.34f;
        const float scaleR = glass.getHeight() * 1.06f;
        constexpr float aStart = -0.66f; // ~ -38 deg
        constexpr float aZero  =  0.24f; // 0 VU
        constexpr float aEnd   =  0.66f; // +3 VU

        auto dbToAngle = [&](float db) -> float
        {
            // Mapeamento nao-linear autentico de escala VU (-20 .. 0 .. +3)
            if (db <= 0.0f)
            {
                const float norm = std::pow(juce::jlimit(0.0f, 1.0f, (db + 20.0f) / 20.0f), 1.35f);
                return aStart + norm * (aZero - aStart);
            }
            const float normPos = juce::jlimit(0.0f, 1.0f, db / 3.0f);
            return aZero + normPos * (aEnd - aZero);
        };

        // Arco principal preto-cafe (-20 a 0 VU) + faixa vermelha (0 a +3 VU)
        juce::Path blackArc, redArc;
        blackArc.addCentredArc(pivotX, pivotY, scaleR, scaleR, 0.0f, aStart, aZero, true);
        redArc.addCentredArc(pivotX, pivotY, scaleR, scaleR, 0.0f, aZero, aEnd, true);

        const juce::Colour inkDark = juce::Colour(0xff2c1a06);
        const juce::Colour inkRed  = juce::Colour(0xffbd1912);

        g.setColour(inkDark);
        g.strokePath(blackArc, juce::PathStrokeType(1.6f));
        g.setColour(inkRed);
        g.strokePath(redArc, juce::PathStrokeType(3.4f));

        // Marcacoes numeradas da escala
        struct ScaleMark { float db; const char* label; bool red; };
        static constexpr ScaleMark kMarks[] = {
            { -20.0f, "20", false }, { -10.0f, "10", false }, { -7.0f, "7", false },
            { -5.0f,  "5",  false }, { -3.0f,  "3",  false }, { -2.0f, "2", false },
            { -1.0f,  "1",  false }, {  0.0f,  "0",  false }, {  1.0f, "1", true  },
            {  2.0f,  "2",  true  }, {  3.0f,  "3",  true  }
        };

        g.setFont(makeFont(11.5f, true, false));
        for (const auto& m : kMarks)
        {
            const float ang = dbToAngle(m.db) - juce::MathConstants<float>::halfPi;
            const float rIn  = scaleR - 1.0f;
            const float rOut = scaleR + 9.5f;
            const float rTxt = scaleR + 19.5f;

            g.setColour(m.red ? inkRed : inkDark);
            g.drawLine(pivotX + std::cos(ang) * rIn,  pivotY + std::sin(ang) * rIn,
                       pivotX + std::cos(ang) * rOut, pivotY + std::sin(ang) * rOut,
                       (m.db == 0.0f) ? 2.2f : 1.4f);

            const float tx = pivotX + std::cos(ang) * rTxt;
            const float ty = pivotY + std::sin(ang) * rTxt;
            g.drawText(m.label, juce::Rectangle<float>(tx - 12.0f, ty - 8.0f, 24.0f, 16.0f),
                       juce::Justification::centred, false);
        }

        // Tipografia central "VU" + modo ativo + assinatura KINGLION
        g.setColour(inkDark.withAlpha(0.88f));
        g.setFont(makeFont(15.0f, true, false));
        g.drawText("VU", juce::Rectangle<float>(pivotX - 24.0f, glass.getBottom() - 44.0f, 48.0f, 20.0f),
                   juce::Justification::centred, false);

        static constexpr const char* kModeNames[5] = { "G-R", "ENT", "PRE", "EQ", "D-S" };
        g.setFont(makeFont(10.5f, true, false));
        g.drawText(kModeNames[modeIdx],
                   juce::Rectangle<float>(glass.getX() + 10.0f, glass.getBottom() - 22.0f, 48.0f, 16.0f),
                   juce::Justification::centredLeft, false);

        g.setFont(makeFont(9.5f, true, true));
        g.setColour(inkDark.withAlpha(0.68f));
        g.drawText("KINGLION",
                   juce::Rectangle<float>(glass.getRight() - 78.0f, glass.getBottom() - 22.0f, 68.0f, 16.0f),
                   juce::Justification::centredRight, false);

        // Ponteiro com sombra suave
        const float needleAngle = (isPowered ? dbToAngle(currentDb) : aStart) - juce::MathConstants<float>::halfPi;
        const float needleLen = scaleR + 12.0f;
        const float tipX = pivotX + std::cos(needleAngle) * needleLen;
        const float tipY = pivotY + std::sin(needleAngle) * needleLen;

        g.setColour(juce::Colours::black.withAlpha(0.28f));
        g.drawLine(pivotX + 2.0f, pivotY + 2.0f, tipX + 2.5f, tipY + 2.5f, 2.2f);

        g.setColour(juce::Colour(0xff1b1208));
        g.drawLine(pivotX, pivotY, tipX, tipY, 1.9f);

        // Calota da base do ponteiro
        g.setColour(juce::Colour(0xff23180c));
        g.fillEllipse(pivotX - 18.0f, glass.getBottom() - 8.0f, 36.0f, 22.0f);

        // Reflexo curvo sutil do vidro superior
        juce::Path gloss;
        gloss.addRectangle(glass.getX(), glass.getY(), glass.getWidth(), glass.getHeight() * 0.34f);
        juce::ColourGradient glossGrad(juce::Colours::white.withAlpha(0.22f), glass.getX(), glass.getY(),
                                       juce::Colours::white.withAlpha(0.0f),  glass.getX(), glass.getY() + glass.getHeight() * 0.36f, false);
        g.setGradientFill(glossGrad);
        g.fillPath(gloss);

        g.restoreState();
    }

private:
    float currentDb = 0.0f;
    int modeIdx = 0;
    bool isPowered = true;
};

} // namespace kinglion::gui
