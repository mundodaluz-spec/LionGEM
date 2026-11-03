#pragma once
#include "CytomicSVF.h"
#include "Oversampler4x.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace kinglion::dsp
{

/**
 * Segundo Circuito Eletro-Optico ELOP Pos-EQ: De-Esser Passivo (3/6/9/12 kHz) ou Limitador 10:1.
 *
 * Modos fieis ao painel KingLion Box / Manley VOXBOX:
 * - DeEsser: Filtro LC sintonizado no sidechain (3 kHz, 6 kHz, 9 kHz ou 12 kHz) acionando a
 *   celula optica rapida (ataque 2 ms, release 500 ms, razao 10:1) com atenuacao hibrida
 *   focada na sibilancia para domar "S/SH" fortes sem deixar a voz fanha ou cuspida.
 * - Off (Desligado): Bypass suave sem cliques.
 * - Limiter (Limitador 10:1): Desliga o filtro de sidechain e atua como um limitador optico
 *   estilo ELOP / LA-2A de banda inteira no final da cadeia (ate 20 dB de reducao).
 * - SoloSidechain ("OUVIR S"): Isola o sinal do detector de sibilancia para calibrar o corte.
 */
class OptoDeEsserLimiter
{
public:
    enum class Mode
    {
        DeEsser = 0,
        Off = 1,
        Limiter = 2
    };

    static constexpr std::array<double, 4> kSibilanceFreqsHz { 3000.0, 6000.0, 9000.0, 12000.0 };

    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        // Especificacao oficial VOXBOX: Attack 2 ms, Release 0.5 s (com memoria optica suave)
        attackCoeff = std::exp(-std::log(9.0) / (0.002 * sampleRate));
        releaseFastCoeff = std::exp(-1.0 / (0.22 * sampleRate));
        releaseSlowCoeff = std::exp(-1.0 / (0.55 * sampleRate));
        reset();
        updateFilters();
    }

    void reset() noexcept
    {
        scBandFilterL.reset();
        scBandFilterR.reset();
        scHighShelfL.reset();
        scHighShelfR.reset();
        dynSplitBandL.reset();
        dynSplitBandR.reset();
        fastGrDb = 0.0;
        slowGrDb = 0.0;
        currentGrDb = 0.0;
        activeFade = (mode != Mode::Off) ? 1.0 : 0.0;
    }

    void setParameters(Mode newMode, int freqIdx0to3, float threshold0to10, bool soloSibilance) noexcept
    {
        mode = newMode;
        listenSibilance = soloSibilance;
        const int clampedFreq = std::clamp(freqIdx0to3, 0, 3);
        thresholdKnob = std::clamp(static_cast<double>(threshold0to10), 0.0, 10.0);
        // 0 = sem acao (+12 dB), 10 = -32 dB
        thresholdDb = 12.0 - thresholdKnob * 4.4;

        if (clampedFreq != freqIdx)
        {
            freqIdx = clampedFreq;
            updateFilters();
        }
    }

    inline void processStereoSample(double& left, double& right, bool stereoLinked) noexcept
    {
        const bool isEngaged = (mode != Mode::Off);
        const double fadeStep = 1.0 / (0.008 * sampleRate);
        activeFade = isEngaged ? std::min(1.0, activeFade + fadeStep)
                               : std::max(0.0, activeFade - fadeStep);

        const double scBandL = scBandFilterL.processSample(scHighShelfL.processSample(left));
        const double scBandR = scBandFilterR.processSample(scHighShelfR.processSample(right));

        // Modo "OUVIR S" permite escutar exatamente a faixa sibilante selecionada
        if (listenSibilance)
        {
            left = scBandL;
            right = scBandR;
            currentGrDb = 0.0;
            return;
        }

        if (activeFade <= 1.0e-6 && !isEngaged)
        {
            fastGrDb *= 0.999;
            slowGrDb *= 0.9995;
            currentGrDb = 0.0;
            return;
        }

        double detL = (mode == Mode::DeEsser) ? std::abs(scBandL) * 1.65 : std::abs(left);
        double detR = (mode == Mode::DeEsser) ? std::abs(scBandR) * 1.65 : std::abs(right);

        if (stereoLinked)
        {
            const double linked = std::max(detL, detR);
            detL = linked;
            detR = linked;
        }

        const double detMax = std::max(detL, detR);
        const double inputDb = 20.0 * std::log10(std::max(1.0e-7, detMax));
        const double overDb = inputDb - thresholdDb;

        // Curva optica 10:1 (slope = 0.90) com joelho suave de 4 dB e teto de 20 dB
        double targetGrDb = 0.0;
        constexpr double knee = 4.0;
        constexpr double slope = 0.90;
        if (overDb > -0.5 * knee)
        {
            if (overDb < 0.5 * knee)
            {
                const double t = overDb + 0.5 * knee;
                targetGrDb = slope * (t * t) / (2.0 * knee);
            }
            else
            {
                targetGrDb = slope * overDb;
            }
        }
        if (targetGrDb > 0.0)
            targetGrDb = 20.0 * (1.0 - std::exp(-targetGrDb / 20.0));

        if (targetGrDb > fastGrDb)
            fastGrDb = attackCoeff * fastGrDb + (1.0 - attackCoeff) * targetGrDb;
        else
            fastGrDb = releaseFastCoeff * fastGrDb + (1.0 - releaseFastCoeff) * targetGrDb;

        if (fastGrDb > slowGrDb)
            slowGrDb = 0.98 * slowGrDb + 0.02 * fastGrDb;
        else
            slowGrDb = releaseSlowCoeff * slowGrDb + (1.0 - releaseSlowCoeff) * fastGrDb;

        const double grDb = std::clamp(0.78 * fastGrDb + 0.22 * slowGrDb, 0.0, 20.0) * activeFade;
        currentGrDb = grDb;

        if (mode == Mode::DeEsser)
        {
            // No modo De-Esser da VOXBOX, o circuito optico atenua de forma muito natural:
            // combinamos 65% de acao dinamica concentrada na banda sibilante + 35% broadband
            // optico para preservar o timbre sedoso de hardware sem soar artificial.
            const double bandL = dynSplitBandL.processSample(left);
            const double bandR = dynSplitBandR.processSample(right);

            const double fullGain = std::pow(10.0, -(grDb * 0.35) / 20.0);
            const double sibAtten = 1.0 - std::pow(10.0, -(grDb * 0.85) / 20.0);

            left  = (left  - sibAtten * bandL) * fullGain;
            right = (right - sibAtten * bandR) * fullGain;
        }
        else
        {
            const double limGain = std::pow(10.0, -grDb / 20.0);
            left  *= limGain;
            right *= limGain;
        }
    }

    double getGainReductionDb() const noexcept { return currentGrDb; }

private:
    void updateFilters() noexcept
    {
        const double fc = kSibilanceFreqsHz[static_cast<size_t>(freqIdx)];
        scHighShelfL.setParameters(CytomicSVF::Type::HighShelf, sampleRate, fc * 0.75, 0.707, 4.5);
        scHighShelfR.setParameters(CytomicSVF::Type::HighShelf, sampleRate, fc * 0.75, 0.707, 4.5);
        scBandFilterL.setParameters(CytomicSVF::Type::BandPass, sampleRate, fc, 1.35, 0.0);
        scBandFilterR.setParameters(CytomicSVF::Type::BandPass, sampleRate, fc, 1.35, 0.0);

        dynSplitBandL.setParameters(CytomicSVF::Type::BandPass, sampleRate, fc, 0.95, 0.0);
        dynSplitBandR.setParameters(CytomicSVF::Type::BandPass, sampleRate, fc, 0.95, 0.0);
    }

    double sampleRate = 48000.0;
    Mode mode = Mode::DeEsser;
    int freqIdx = 1; // 6 kHz padrao vocal
    double thresholdKnob = 0.0;
    double thresholdDb = 12.0;
    bool listenSibilance = false;

    double attackCoeff = 0.98;
    double releaseFastCoeff = 0.9999;
    double releaseSlowCoeff = 0.99996;
    double fastGrDb = 0.0;
    double slowGrDb = 0.0;
    double currentGrDb = 0.0;
    double activeFade = 1.0;

    CytomicSVF scBandFilterL, scBandFilterR;
    CytomicSVF scHighShelfL, scHighShelfR;
    CytomicSVF dynSplitBandL, dynSplitBandR;
};

/**
 * Transformador de Saida com Nucleo de Niquel/Ferro Estilo Manley IRON (Chave XFMR).
 * Adiciona peso sutil nos subgraves (~48 Hz) e saturacao magnetica dependente de frequencia
 * (maior fluxo magnetico em frequencias graves) processada em 4x oversampling.
 */
class IronTransformer
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        // Ressonancia primaria suave de transformador acoplado (head-bump sutil em 48 Hz)
        lfBump.setParameters(CytomicSVF::Type::Bell, sampleRate, 48.0, 0.65, 0.45);
        hfSilk.setParameters(CytomicSVF::Type::HighShelf, sampleRate, 18500.0, 0.60, -0.30);
        fluxLpCoeff = std::exp(-2.0 * 3.14159265358979323846 * 130.0 / (sampleRate * 4.0));
        reset();
    }

    void reset() noexcept
    {
        lfBump.reset();
        hfSilk.reset();
        oversampler.reset();
        fluxState = 0.0;
        xfmrFade = engaged ? 1.0 : 0.0;
    }

    void setEngaged(bool isEngaged) noexcept { engaged = isEngaged; }

    inline double processSample(double x) noexcept
    {
        const double fadeStep = 1.0 / (0.008 * sampleRate);
        xfmrFade = engaged ? std::min(1.0, xfmrFade + fadeStep)
                           : std::max(0.0, xfmrFade - fadeStep);

        if (xfmrFade <= 1.0e-6 && !engaged)
            return x;

        double y = lfBump.processSample(x);
        const double lpC = fluxLpCoeff;

        y = oversampler.processSample(y, [&](double u) noexcept -> double
        {
            // Fluxo magnetico integra mais energia nos graves, gerando 3o harmonico quente no peso da voz/baixo
            fluxState = lpC * fluxState + (1.0 - lpC) * u;
            const double coreMag = fluxState * 0.55;
            const double hyst = u + 0.085 * (coreMag - (coreMag * coreMag * coreMag) * 0.333333);
            return std::tanh(hyst * 0.88) / 0.88;
        });

        y = hfSilk.processSample(y);
        return x + xfmrFade * (y - x);
    }

private:
    double sampleRate = 48000.0;
    bool engaged = true;
    double xfmrFade = 1.0;
    double fluxLpCoeff = 0.99;
    double fluxState = 0.0;

    CytomicSVF lfBump;
    CytomicSVF hfSilk;
    Oversampler4x oversampler;
};

} // namespace kinglion::dsp
