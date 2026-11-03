#pragma once
#include "VactrolOptoCompressor.h"
#include "ClassATubePreamp.h"
#include "PultecPassiveEQ.h"
#include "OptoDeEsserLimiter.h"
#include <atomic>
#include <cmath>
#include <algorithm>

namespace kinglion::dsp
{

/**
 * Balistica Real de Medidor VU Analogico Sifam (300 ms de integracao)
 * Suporta as 5 posicoes da VOXBOX / KingLion Box:
 * 0: G-R (Reducao de ganho do Compressor Optico 3:1)
 * 1: ENT (Nivel de entrada pos-trim, calibrado para 0 VU = -18 dBFS)
 * 2: PRE (Nivel de saida do Pre Valvulado Classe A)
 * 3: EQ  (Nivel de saida do EQ Passivo Pultec)
 * 4: D-S (Reducao de ganho do De-Esser / Limitador 10:1)
 */
class VuMeterBallistics
{
public:
    enum class Source
    {
        CompGR = 0,
        InputLevel = 1,
        PreampOut = 2,
        EqOut = 3,
        DeEssGR = 4
    };

    void prepare(double sampleRate) noexcept
    {
        const double sr = std::max(8000.0, sampleRate);
        // Constante de integracao classica de ponteiro VU (~300 ms para 99%)
        vuRiseCoeff = std::exp(-1.0 / (0.065 * sr));
        vuFallCoeff = std::exp(-1.0 / (0.085 * sr));
        grRiseCoeff = std::exp(-1.0 / (0.025 * sr));
        grFallCoeff = std::exp(-1.0 / (0.095 * sr));
        reset();
    }

    void reset() noexcept
    {
        needleDb = -20.0;
    }

    inline void pushMeasurement(Source src, double levelLinear, double grDb) noexcept
    {
        double targetNeedleDb = -20.0;
        const bool isGainReduction = (src == Source::CompGR || src == Source::DeEssGR);

        if (isGainReduction)
        {
            // Em modo G-R / D-S, 0 VU e o repouso na direita (0 dB GR) e deflexiona para a esquerda (-dB)
            targetNeedleDb = -std::clamp(grDb, 0.0, 20.0);
            const double c = (targetNeedleDb < needleDb) ? grRiseCoeff : grFallCoeff;
            needleDb = c * needleDb + (1.0 - c) * targetNeedleDb;
        }
        else
        {
            // Calibracao padrao de estudio: -18 dBFS RMS/medio = 0 VU (+4 dBu)
            const double dbfs = 20.0 * std::log10(std::max(1.0e-5, levelLinear));
            targetNeedleDb = std::clamp(dbfs + 18.0, -20.0, 3.5);
            const double c = (targetNeedleDb > needleDb) ? vuRiseCoeff : vuFallCoeff;
            needleDb = c * needleDb + (1.0 - c) * targetNeedleDb;
        }
    }

    double getNeedleVuDb() const noexcept { return needleDb; }

private:
    double vuRiseCoeff = 0.999;
    double vuFallCoeff = 0.999;
    double grRiseCoeff = 0.998;
    double grFallCoeff = 0.999;
    double needleDb = -20.0;
};

struct KingLionParameters
{
    bool powerOn = true;

    // Secao 1: Pre Valvulado
    float inputTrimDb = 0.0f;      // -24 a +12 dB
    int lowCutMode = 0;            // 0: Flat, 1: 80 Hz, 2: 120 Hz
    bool phaseInvert = false;      // false: 0 deg, true: 180 deg
    int preampGainStep = 2;        // 0: 40dB, 1: 45dB, 2: 50dB, 3: 55dB, 4: 60dB

    // Secao 2: Compressor Optico 3:1 (Pre-Preamp!)
    bool compEngaged = true;
    bool stereoLink = true;
    float compThreshold = 4.0f;    // 0 a 10
    int compAttackStep = 2;        // 0: Slow .. 4: Fast
    int compReleaseStep = 1;       // 0: Slow (5s) .. 4: Fast (0.3s)

    // Secao 3: EQ Passivo Estilo Pultec (33 frequencias)
    bool eqEngaged = true;
    int lowFreqStep = 4;           // 0..10 (padrao 4 = 100 Hz)
    float lowBoostDb = 0.0f;       // 0 a +10 dB
    int midFreqStep = 2;           // 0..10 (padrao 2 = 500 Hz)
    float midDipDb = 0.0f;         // 0 a +10 dB de corte (-dB)
    int highFreqStep = 8;          // 0..10 (padrao 8 = 12 kHz)
    float highBoostDb = 0.0f;      // 0 a +10 dB

    // Secao 4: De-Esser / Limitador Optico 10:1 + Saida
    int deEssMode = 0;             // 0: De-Esser, 1: Desligado, 2: Limitador 10:1
    int deEssFreqStep = 1;         // 0: 3k, 1: 6k, 2: 9k, 3: 12k
    float deEssThreshold = 0.0f;   // 0 a 10
    bool soloSibilance = false;    // false: Normal, true: Ouvir S
    bool xfmrEngaged = true;       // Transformador de saida Manley IRON
    float outputTrimDb = 0.0f;     // -24 a +12 dB

    // Medidor VU (5 leituras)
    int meterMode = 0;             // 0: G-R, 1: ENT, 2: PRE, 3: EQ, 4: D-S
};

/**
 * Cadeia Completa Stereo/Mono do KingLion Box 1.0 — WYATT Sounds.
 * Orquestra rigorosamente a topologia fisica da Manley VOXBOX:
 * Entrada/Fase/HPF -> Fotocelula Vactrol 3:1 -> Valvulas Classe A (12AX7/6414 c/ NFB)
 * -> EQ Passivo LC 33 Bandas -> De-Esser/Limiter Optico 10:1 -> Transformador -> Saida.
 */
class KingLionChannelStrip
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        for (int ch = 0; ch < 2; ++ch)
        {
            preamp[ch].prepare(sampleRate);
            comp[ch].prepare(sampleRate);
            eq[ch].prepare(sampleRate);
            xfmr[ch].prepare(sampleRate);
        }
        deEsserLimiter.prepare(sampleRate);
        vuMeter.prepare(sampleRate);
        outSmoothAlpha = 1.0 - std::exp(-1.0 / (0.010 * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            preamp[ch].reset();
            comp[ch].reset();
            eq[ch].reset();
            xfmr[ch].reset();
        }
        deEsserLimiter.reset();
        vuMeter.reset();
        powerFade = params.powerOn ? 1.0 : 0.0;
        smoothedOutGain = targetOutGain;
    }

    void updateParameters(const KingLionParameters& p) noexcept
    {
        params = p;
        const auto cutEnum = (p.lowCutMode == 2) ? ClassATubePreamp::LowCutMode::Hz120
                           : (p.lowCutMode == 1) ? ClassATubePreamp::LowCutMode::Hz80
                                                 : ClassATubePreamp::LowCutMode::Flat;

        for (int ch = 0; ch < 2; ++ch)
        {
            preamp[ch].setInputParameters(p.inputTrimDb, cutEnum, p.phaseInvert);
            preamp[ch].setFeedbackGainIndex(p.preampGainStep);
            comp[ch].setParameters(p.compEngaged, p.compThreshold, p.compAttackStep, p.compReleaseStep);
            eq[ch].setParameters(p.eqEngaged,
                                 p.lowFreqStep, p.lowBoostDb,
                                 p.midFreqStep, p.midDipDb,
                                 p.highFreqStep, p.highBoostDb);
            xfmr[ch].setEngaged(p.xfmrEngaged);
        }

        const auto dsMode = (p.deEssMode == 0) ? OptoDeEsserLimiter::Mode::DeEsser
                          : (p.deEssMode == 2) ? OptoDeEsserLimiter::Mode::Limiter
                                               : OptoDeEsserLimiter::Mode::Off;
        deEsserLimiter.setParameters(dsMode, p.deEssFreqStep, p.deEssThreshold, p.soloSibilance);

        targetOutGain = std::pow(10.0, std::clamp(static_cast<double>(p.outputTrimDb), -24.0, 12.0) / 20.0);
    }

    void processBlock(float* leftChannel, float* rightChannel, int numSamples) noexcept
    {
        const bool isStereo = (rightChannel != nullptr && rightChannel != leftChannel);
        const double fadeStep = 1.0 / (0.012 * sampleRate);
        const auto meterSrc = static_cast<VuMeterBallistics::Source>(std::clamp(params.meterMode, 0, 4));

        double blockCompGrMax = 0.0;
        double blockDeEssGrMax = 0.0;
        double blockPeakOut = 0.0;

        for (int i = 0; i < numSamples; ++i)
        {
            const double dryL = static_cast<double>(leftChannel[i]);
            const double dryR = isStereo ? static_cast<double>(rightChannel[i]) : dryL;

            powerFade = params.powerOn ? std::min(1.0, powerFade + fadeStep)
                                       : std::max(0.0, powerFade - fadeStep);

            if (powerFade <= 1.0e-6 && !params.powerOn)
            {
                vuMeter.pushMeasurement(meterSrc, 0.0, 0.0);
                continue;
            }

            // 1. Condicionamento de entrada (Trim, Fase 0/180, Low Cut 6 dB/oct)
            double sigL = preamp[0].processInputConditioning(dryL);
            double sigR = isStereo ? preamp[1].processInputConditioning(dryR) : sigL;
            const double entMeterLvl = isStereo ? 0.5 * (std::abs(sigL) + std::abs(sigR)) : std::abs(sigL);

            // 2. Fotocelula Vactrol 3:1 ANTES do estagio de valvulas!
            double detL = std::abs(sigL);
            double detR = std::abs(sigR);
            if (!isStereo || params.stereoLink)
            {
                const double linked = isStereo ? std::max(detL, detR) : detL;
                detL = linked;
                detR = linked;
            }

            const double compGainL = comp[0].computeGainMultiplier(detL);
            const double compGainR = isStereo ? comp[1].computeGainMultiplier(detR) : compGainL;
            sigL *= compGainL;
            sigR *= compGainR;

            const double compGrDb = isStereo
                ? std::max(comp[0].getGainReductionDb(), comp[1].getGainReductionDb())
                : comp[0].getGainReductionDb();
            blockCompGrMax = std::max(blockCompGrMax, compGrDb);

            // 3. Pre-Amplificador Valvulado Classe A (12AX7 + 6414 c/ Realimentacao Negativa GANHO dB)
            sigL = preamp[0].processTubeStage(sigL);
            sigR = isStereo ? preamp[1].processTubeStage(sigR) : sigL;
            const double preMeterLvl = isStereo ? 0.5 * (std::abs(sigL) + std::abs(sigR)) : std::abs(sigL);

            // 4. EQ Passivo LC Estilo Pultec (33 frequencias + estagio valvulado 5751/6414)
            sigL = eq[0].processSample(sigL);
            sigR = isStereo ? eq[1].processSample(sigR) : sigL;
            const double eqMeterLvl = isStereo ? 0.5 * (std::abs(sigL) + std::abs(sigR)) : std::abs(sigL);

            // 5. De-Esser Passivo (3/6/9/12 kHz) / Limitador Optico 10:1 ELOP
            deEsserLimiter.processStereoSample(sigL, sigR, !isStereo || params.stereoLink);
            const double dsGrDb = deEsserLimiter.getGainReductionDb();
            blockDeEssGrMax = std::max(blockDeEssGrMax, dsGrDb);

            // 6. Transformador de Saida Manley IRON (XFMR) + Trim SAIDA
            sigL = xfmr[0].processSample(sigL);
            sigR = isStereo ? xfmr[1].processSample(sigR) : sigL;

            smoothedOutGain += outSmoothAlpha * (targetOutGain - smoothedOutGain);
            sigL *= smoothedOutGain;
            sigR *= smoothedOutGain;

            // Protecao contra subnormais/NaN e crossfade mestre LIGA/DESL.
            if (!std::isfinite(sigL)) sigL = 0.0;
            if (!std::isfinite(sigR)) sigR = 0.0;

            const double outL = dryL + powerFade * (sigL - dryL);
            const double outR = dryR + powerFade * (sigR - dryR);

            leftChannel[i] = static_cast<float>(outL);
            if (isStereo)
                rightChannel[i] = static_cast<float>(outR);

            blockPeakOut = std::max(blockPeakOut, std::max(std::abs(outL), std::abs(outR)));

            // 7. Atualiza balistica do Medidor VU Sifam
            double meterLevel = 0.0;
            double meterGr = 0.0;
            switch (meterSrc)
            {
                case VuMeterBallistics::Source::CompGR:     meterGr = compGrDb; break;
                case VuMeterBallistics::Source::InputLevel: meterLevel = entMeterLvl; break;
                case VuMeterBallistics::Source::PreampOut:  meterLevel = preMeterLvl; break;
                case VuMeterBallistics::Source::EqOut:      meterLevel = eqMeterLvl; break;
                case VuMeterBallistics::Source::DeEssGR:    meterGr = dsGrDb; break;
            }
            vuMeter.pushMeasurement(meterSrc, meterLevel, meterGr);
        }

        latestVuNeedleDb.store(static_cast<float>(vuMeter.getNeedleVuDb()), std::memory_order_relaxed);
        latestCompGrDb.store(static_cast<float>(blockCompGrMax), std::memory_order_relaxed);
        latestDeEssGrDb.store(static_cast<float>(blockDeEssGrMax), std::memory_order_relaxed);
        latestOutputPeak.store(static_cast<float>(blockPeakOut), std::memory_order_relaxed);
    }

    float getVuNeedleDb() const noexcept   { return latestVuNeedleDb.load(std::memory_order_relaxed); }
    float getCompGrDb() const noexcept     { return latestCompGrDb.load(std::memory_order_relaxed); }
    float getDeEssGrDb() const noexcept    { return latestDeEssGrDb.load(std::memory_order_relaxed); }
    float getOutputPeak() const noexcept   { return latestOutputPeak.load(std::memory_order_relaxed); }

private:
    double sampleRate = 48000.0;
    KingLionParameters params {};

    ClassATubePreamp preamp[2];
    VactrolOptoCompressor comp[2];
    PultecPassiveEQ eq[2];
    OptoDeEsserLimiter deEsserLimiter;
    IronTransformer xfmr[2];
    VuMeterBallistics vuMeter;

    double powerFade = 1.0;
    double targetOutGain = 1.0;
    double smoothedOutGain = 1.0;
    double outSmoothAlpha = 0.002;

    std::atomic<float> latestVuNeedleDb { 0.0f };
    std::atomic<float> latestCompGrDb   { 0.0f };
    std::atomic<float> latestDeEssGrDb  { 0.0f };
    std::atomic<float> latestOutputPeak { 0.0f };
};

} // namespace kinglion::dsp
