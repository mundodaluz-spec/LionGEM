#pragma once
#include "CytomicSVF.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace kinglion::dsp
{

/**
 * Equalizador Passivo Indutor-Capacitor (LC) Estilo Pultec MEQ-5 Estendido (33 Frequencias)
 * + Estagio de Recuperacao Valvulado Classe A (5751 + 6414).
 *
 * Replica todas as 33 frequencias reais da Manley VOXBOX:
 * - GRAVE (Low Peak, 0 a +10 dB): 20, 35, 50, 70, 100, 150, 200, 300, 500, 700, 1000 Hz
 * - MEDIO (Mid Dip,  0 a -10 dB): 200, 300, 500, 700, 1k, 1.5k, 2k, 3k, 4k, 5k, 7k Hz
 * - AGUDO (Hi Peak,  0 a +10 dB): 1.5k, 2k, 3k, 4k, 5k, 6.4k, 8k, 10k, 12k, 16k, 20k Hz
 *
 * As 3 redes tanque LC interagem organicamente com Q proporcional (curvas largas em ajustes
 * sutis que focam suavemente ao abrir o potenciometro), permitindo o classico "truque Pultec"
 * de empurrar Grave e cortar Medio adjacente para limpar embolamento sem perder peso.
 */
class PultecPassiveEQ
{
public:
    static constexpr std::array<double, 11> kLowFreqsHz {
        20.0, 35.0, 50.0, 70.0, 100.0, 150.0, 200.0, 300.0, 500.0, 700.0, 1000.0
    };

    static constexpr std::array<double, 11> kMidFreqsHz {
        200.0, 300.0, 500.0, 700.0, 1000.0, 1500.0, 2000.0, 3000.0, 4000.0, 5000.0, 7000.0
    };

    static constexpr std::array<double, 11> kHighFreqsHz {
        1500.0, 2000.0, 3000.0, 4000.0, 5000.0, 6400.0, 8000.0, 10000.0, 12000.0, 16000.0, 20000.0
    };

    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        dcBlock.setCutoff(sampleRate, 5.0);
        reset();
        updateFilters();
    }

    void reset() noexcept
    {
        lowTank.reset();
        midDipTank.reset();
        highTank.reset();
        dcBlock.reset();
        bypassFade = engaged ? 1.0 : 0.0;
    }

    void setParameters(bool isEngaged,
                       int lowFreqIdx0to10, float lowBoost0to10Db,
                       int midFreqIdx0to10, float midDip0to10Db,
                       int highFreqIdx0to10, float highBoost0to10Db) noexcept
    {
        engaged = isEngaged;
        const int lf = std::clamp(lowFreqIdx0to10, 0, 10);
        const int mf = std::clamp(midFreqIdx0to10, 0, 10);
        const int hf = std::clamp(highFreqIdx0to10, 0, 10);

        const double lb = std::clamp(static_cast<double>(lowBoost0to10Db), 0.0, 10.0);
        const double md = std::clamp(static_cast<double>(midDip0to10Db), 0.0, 10.0);
        const double hb = std::clamp(static_cast<double>(highBoost0to10Db), 0.0, 10.0);

        if (lf != lowIdx || mf != midIdx || hf != highIdx
            || std::abs(lb - lowBoostDb) > 1.0e-4
            || std::abs(md - midDipDb) > 1.0e-4
            || std::abs(hb - highBoostDb) > 1.0e-4)
        {
            lowIdx = lf;
            midIdx = mf;
            highIdx = hf;
            lowBoostDb = lb;
            midDipDb = md;
            highBoostDb = hb;
            updateFilters();
        }
    }

    inline double processSample(double x) noexcept
    {
        const double fadeStep = 1.0 / (0.008 * sampleRate);
        bypassFade = engaged ? std::min(1.0, bypassFade + fadeStep)
                             : std::max(0.0, bypassFade - fadeStep);

        if (bypassFade <= 1.0e-6 && !engaged)
            return x;

        // Rede LC passiva: Low Peak + Mid Dip + High Peak
        double y = lowTank.processSample(x);
        y = midDipTank.processSample(y);
        y = highTank.processSample(y);

        // Estagio de recuperacao de ganho valvulado 5751 + 6414 (calor sutil de 2a harmonica)
        constexpr double tubeBias = 0.06;
        const double sat = (std::tanh(y * 0.65 + tubeBias) - std::tanh(tubeBias)) / 0.65;
        y = dcBlock.processSample(0.82 * y + 0.18 * sat);

        return x + bypassFade * (y - x);
    }

private:
    void updateFilters() noexcept
    {
        const double fLow  = kLowFreqsHz[static_cast<size_t>(lowIdx)];
        const double fMid  = kMidFreqsHz[static_cast<size_t>(midIdx)];
        const double fHigh = kHighFreqsHz[static_cast<size_t>(highIdx)];

        // Curvas indutor-capacitor proporcionais estilo Pultec MEQ-5:
        // Low Peak: Q largo e musical (0.48 -> 0.78) para grave encorpado sem ressonancia oca
        const double qLow = 0.48 + 0.030 * lowBoostDb;
        lowTank.setParameters(CytomicSVF::Type::Bell, sampleRate, fLow, qLow, lowBoostDb);

        // Mid Dip: tanque LC serie com Q mais focado conforme aprofunda o corte (0.85 -> 1.75)
        const double qMid = 0.85 + 0.090 * midDipDb;
        midDipTank.setParameters(CytomicSVF::Type::Bell, sampleRate, fMid, qMid, -midDipDb);

        // High Peak: Q sedoso de indutor de ar (0.45 -> 0.72), aberto ate 20 kHz sem aspereza
        const double qHigh = 0.45 + 0.027 * highBoostDb;
        highTank.setParameters(CytomicSVF::Type::Bell, sampleRate, fHigh, qHigh, highBoostDb);
    }

    double sampleRate = 48000.0;
    bool engaged = true;
    int lowIdx = 4;   // 100 Hz
    int midIdx = 2;   // 500 Hz
    int highIdx = 8;  // 12 kHz
    double lowBoostDb = 0.0;
    double midDipDb = 0.0;
    double highBoostDb = 0.0;
    double bypassFade = 1.0;

    CytomicSVF lowTank;
    CytomicSVF midDipTank;
    CytomicSVF highTank;
    FirstOrderHighPass dcBlock;
};

} // namespace kinglion::dsp
