#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace kinglion::dsp
{

/**
 * Compressor Eletro-Optico Vactrol Pre-Preamp (inspirado na topologia ELOP/VOXBOX).
 *
 * Pontos-chave da engenharia original:
 * 1. Posicionado ANTES do estagio de valvulas do pre-amplificador, segurando picos antes
 *    de saturar a grade da 12AX7.
 * 2. Razao suave dependente de programa em torno de 3:1 (joelho organico de fotocelula)
 *    com teto natural de 16 dB de reducao de ganho.
 * 3. Cinco posicoes de Attack (Slow 70 ms -> Fast 4 ms) e cinco de Release
 *    (Slow 5.0 s, Med Slow 2.0 s, Medium 1.0 s, Med Fast 0.5 s, Fast 0.3 s).
 * 4. Memoria optica dupla (cauda rapida + cauda lenta CdS que desacelera ao se aproximar
 *    de 0 dB) e acoplamento real entre Release e Attack.
 */
class VactrolOptoCompressor
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        reset();
        updateTimeConstants();
    }

    void reset() noexcept
    {
        fastEnvDb = 0.0;
        slowMemoryDb = 0.0;
        currentGainReductionDb = 0.0;
        bypassFade = engaged ? 1.0 : 0.0;
    }

    void setParameters(bool isEngaged, float threshold0to10, int attackIndex0to4, int releaseIndex0to4) noexcept
    {
        engaged = isEngaged;
        // Threshold 0 = sem compressao (+12 dBFS); Threshold 10 = -34 dBFS
        thresholdKnob = std::clamp(static_cast<double>(threshold0to10), 0.0, 10.0);
        thresholdDb = 12.0 - thresholdKnob * 4.6;

        const int clampedAtk = std::clamp(attackIndex0to4, 0, 4);
        const int clampedRel = std::clamp(releaseIndex0to4, 0, 4);
        if (clampedAtk != attackIdx || clampedRel != releaseIdx)
        {
            attackIdx = clampedAtk;
            releaseIdx = clampedRel;
            updateTimeConstants();
        }
    }

    inline double computeGainMultiplier(double detectorAbsLevel) noexcept
    {
        const double fadeStep = 1.0 / (0.008 * sampleRate);
        bypassFade = engaged ? std::min(1.0, bypassFade + fadeStep)
                             : std::max(0.0, bypassFade - fadeStep);

        if (bypassFade <= 1.0e-6 && !engaged)
        {
            fastEnvDb *= 0.999;
            slowMemoryDb *= 0.9995;
            currentGainReductionDb = 0.0;
            return 1.0;
        }

        const double inputDb = 20.0 * std::log10(std::max(1.0e-7, detectorAbsLevel));
        const double overDb = inputDb - thresholdDb;

        // Joelho optico suave (soft-knee de 8 dB) convergindo para razao ~3:1
        double targetGrDb = 0.0;
        constexpr double kneeWidth = 8.0;
        constexpr double slope = 1.0 - (1.0 / 3.0); // 0.6667 para 3:1

        if (overDb > -0.5 * kneeWidth)
        {
            if (overDb < 0.5 * kneeWidth)
            {
                const double t = overDb + 0.5 * kneeWidth;
                targetGrDb = slope * (t * t) / (2.0 * kneeWidth);
            }
            else
            {
                targetGrDb = slope * overDb;
            }
        }

        // Saturacao suave da fotocelula no limite fisico de 16.0 dB
        if (targetGrDb > 0.0)
            targetGrDb = 16.0 * (1.0 - std::exp(-targetGrDb / 16.0));

        // Interacao Vactrol: quando a celula ja possui memoria optica (slowMemoryDb),
        // o ataque efetivo "flutua" sobre o piso de sustentacao do release.
        if (targetGrDb > fastEnvDb)
        {
            const double memoryBoost = 1.0 + 0.35 * (slowMemoryDb / 16.0);
            const double effAtkCoeff = std::exp(-std::log(9.0) / (std::max(1.0, attackSamples / memoryBoost)));
            fastEnvDb = effAtkCoeff * fastEnvDb + (1.0 - effAtkCoeff) * targetGrDb;
        }
        else
        {
            // Desaceleracao progressiva conforme a reducao se aproxima de 0 dB (efeito CdS)
            const double tailSlowdown = 1.0 + 1.6 * std::exp(-fastEnvDb * 0.45);
            const double effRelCoeff = std::exp(-1.0 / (releaseFastSamples * tailSlowdown));
            fastEnvDb = effRelCoeff * fastEnvDb + (1.0 - effRelCoeff) * targetGrDb;
        }

        // Segundo estagio de memoria optica lenta (efeito electroluminescente/Vactrol)
        if (fastEnvDb > slowMemoryDb)
            slowMemoryDb = memChargeCoeff * slowMemoryDb + (1.0 - memChargeCoeff) * fastEnvDb;
        else
            slowMemoryDb = memDischargeCoeff * slowMemoryDb + (1.0 - memDischargeCoeff) * fastEnvDb;

        const double rawGrDb = std::clamp(0.72 * fastEnvDb + 0.28 * slowMemoryDb, 0.0, 16.0);
        currentGainReductionDb = rawGrDb * bypassFade;

        // Perda de insercao passiva minima de 0.1 dB quando engatado (fiel ao circuito real)
        const double insertionLossDb = 0.1 * bypassFade;
        return std::pow(10.0, -(currentGainReductionDb + insertionLossDb) / 20.0);
    }

    double getGainReductionDb() const noexcept { return currentGainReductionDb; }

private:
    void updateTimeConstants() noexcept
    {
        // Attack: Slow (70 ms), Med Slow (38 ms), Medium (18 ms), Med Fast (9 ms), Fast (4 ms)
        static constexpr std::array<double, 5> kAttackMs  { 70.0, 38.0, 18.0, 9.0, 4.0 };
        // Release oficial VOXBOX: Slow (5.0 s), Med Slow (2.0 s), Medium (1.0 s), Med Fast (0.5 s), Fast (0.3 s)
        static constexpr std::array<double, 5> kReleaseSec { 5.0,  2.0,  1.0,  0.5, 0.3 };

        // Interatividade passiva: Release lento alonga sutilmente o tempo efetivo de carga
        const double relInteraction = 0.88 + 0.06 * (4 - releaseIdx);
        attackSamples = (kAttackMs[static_cast<size_t>(attackIdx)] * 0.001 * relInteraction) * sampleRate;

        const double relSec = kReleaseSec[static_cast<size_t>(releaseIdx)];
        releaseFastSamples = (relSec * 0.42) * sampleRate;

        memChargeCoeff    = std::exp(-1.0 / (0.18 * sampleRate));
        memDischargeCoeff = std::exp(-1.0 / (relSec * 1.65 * sampleRate));
    }

    double sampleRate = 48000.0;
    bool engaged = true;
    double thresholdKnob = 4.0;
    double thresholdDb = -6.4;
    int attackIdx = 2;
    int releaseIdx = 1;

    double attackSamples = 864.0;
    double releaseFastSamples = 40320.0;
    double memChargeCoeff = 0.9998;
    double memDischargeCoeff = 0.99998;

    double fastEnvDb = 0.0;
    double slowMemoryDb = 0.0;
    double currentGainReductionDb = 0.0;
    double bypassFade = 1.0;
};

} // namespace kinglion::dsp
