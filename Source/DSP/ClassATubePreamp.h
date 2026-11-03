#pragma once
#include "CytomicSVF.h"
#include "Oversampler4x.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace kinglion::dsp
{

/**
 * Estagio Pre-Amplificador Valvulado Classe A (12AX7 + 6414) com Realimentacao Negativa.
 *
 * Na VOXBOX original, a chave escalonada GANHO (40, 45, 50, 55, 60 dB) NAO e um pad comum:
 * ela altera a malha de realimentacao negativa (negative feedback) do circuito valvulado.
 * - Em 40 dB (alto feedback negativo): altissimo headroom, graves firmes e THD quase nulo.
 * - Subindo ate 60 dB (menor feedback negativo): abre harmonicos pares (2a ordem) ricos,
 *   adiciona compressao natural de grade e projeta os medios-graves vocais para frente,
 *   com compensacao calibrada de nivel para facilitar o "gain staging" com o knob ENTRADA.
 */
class ClassATubePreamp
{
public:
    enum class LowCutMode
    {
        Flat = 0,
        Hz80 = 1,
        Hz120 = 2
    };

    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        hpf80.setCutoff(sampleRate, 80.0);
        hpf120.setCutoff(sampleRate, 120.0);
        dcBlocker.setCutoff(sampleRate, 6.5);

        // Suavizacao sutil de capacitancia de placa/Miller nos ultra-agudos conforme o feedback muda
        hfToneFilter.setParameters(CytomicSVF::Type::HighShelf, sampleRate, 14500.0, 0.55, -0.35);

        smoothAlpha = 1.0 - std::exp(-1.0 / (0.010 * sampleRate));
        biasChargeAlpha = 1.0 - std::exp(-1.0 / (0.004 * sampleRate * 4.0));
        biasReleaseAlpha = 1.0 - std::exp(-1.0 / (0.065 * sampleRate * 4.0));
        reset();
    }

    void reset() noexcept
    {
        hpf80.reset();
        hpf120.reset();
        dcBlocker.reset();
        hfToneFilter.reset();
        oversampler.reset();
        gridBiasShift = 0.0;
        smoothedInputGain = targetInputGain;
    }

    void setInputParameters(float inputTrimDb, LowCutMode cutMode, bool invertPolarity) noexcept
    {
        const double clampedTrim = std::clamp(static_cast<double>(inputTrimDb), -24.0, 12.0);
        targetInputGain = std::pow(10.0, clampedTrim / 20.0);
        lowCutMode = cutMode;
        polaritySign = invertPolarity ? -1.0 : 1.0;
    }

    void setFeedbackGainIndex(int gainIndex0to4) noexcept
    {
        const int idx = std::clamp(gainIndex0to4, 0, 4);
        if (idx == gainStepIdx)
            return;

        gainStepIdx = idx;
        // 0: 40dB, 1: 45dB, 2: 50dB, 3: 55dB, 4: 60dB
        static constexpr std::array<double, 5> kDriveScale     { 0.55, 0.85, 1.25, 1.85, 2.75 };
        static constexpr std::array<double, 5> kFeedbackAmount { 0.82, 0.64, 0.46, 0.28, 0.14 };
        static constexpr std::array<double, 5> kForwardPushDb  { -1.5, 0.0,  1.8,  3.6,  5.5  };
        static constexpr std::array<double, 5> kAirPresenceDb  { -0.2, 0.0,  0.35, 0.65, 0.95 };

        drive = kDriveScale[static_cast<size_t>(idx)];
        feedback = kFeedbackAmount[static_cast<size_t>(idx)];
        makeupCompensation = std::pow(10.0, kForwardPushDb[static_cast<size_t>(idx)] / 20.0);
        hfToneFilter.setParameters(CytomicSVF::Type::HighShelf, sampleRate, 11500.0, 0.55,
                                   kAirPresenceDb[static_cast<size_t>(idx)]);
    }

    // Etapa 1: Entrada, Polaridade e Low Cut 6 dB/oitava (ocorre ANTES da fotocelula Vactrol)
    inline double processInputConditioning(double x) noexcept
    {
        smoothedInputGain += smoothAlpha * (targetInputGain - smoothedInputGain);
        double sig = x * polaritySign * smoothedInputGain;

        if (lowCutMode == LowCutMode::Hz80)
            sig = hpf80.processSample(sig);
        else if (lowCutMode == LowCutMode::Hz120)
            sig = hpf120.processSample(sig);

        return sig;
    }

    // Etapa 2: Amplificacao Valvulada Classe A 12AX7 + 6414 (ocorre DEPOIS do Vactrol)
    inline double processTubeStage(double x) noexcept
    {
        const double drv = drive;
        const double fb = feedback;

        double y = oversampler.processSample(x, [&](double u) noexcept -> double
        {
            const double excited = u * drv;

            // Deslocamento dinamico sutil de polarizacao de catodo/grade nos picos positivos
            if (excited > 0.35)
                gridBiasShift += biasChargeAlpha * ((excited - 0.35) * 0.22 - gridBiasShift);
            else
                gridBiasShift -= biasReleaseAlpha * gridBiasShift;

            const double vGrid = excited - gridBiasShift * (1.0 - 0.5 * fb);

            // Curva de transferencia assimetrica Triodo Classe A (2o harmonico rico + 3o suave)
            // Amortecida pela malha de realimentacao negativa (fb alto = muito mais linear)
            constexpr double asymOffset = 0.14;
            const double nlPos = std::tanh(vGrid + asymOffset) - std::tanh(asymOffset);
            const double evenComponent = 0.11 * (1.0 - fb) * (vGrid * vGrid) / (1.0 + vGrid * vGrid);

            const double openLoop = nlPos + evenComponent;
            const double linearRef = vGrid;
            const double closedLoop = fb * linearRef + (1.0 - fb) * openLoop;

            // Seguidor de catodo 6414 (arredondamento suave de pico sem corte brusco)
            const double norm = 1.0 / std::max(0.35, drv * (fb + (1.0 - fb) * 0.92));
            return std::tanh(closedLoop * norm * 0.92) / 0.92;
        });

        y = dcBlocker.processSample(y);
        y = hfToneFilter.processSample(y);
        return y * makeupCompensation;
    }

private:
    double sampleRate = 48000.0;
    double smoothAlpha = 0.002;
    double biasChargeAlpha = 0.001;
    double biasReleaseAlpha = 0.0001;

    double targetInputGain = 1.0;
    double smoothedInputGain = 1.0;
    double polaritySign = 1.0;
    LowCutMode lowCutMode = LowCutMode::Flat;

    int gainStepIdx = 2; // Padrao: 50 dB
    double drive = 1.25;
    double feedback = 0.46;
    double makeupCompensation = 1.23;
    double gridBiasShift = 0.0;

    FirstOrderHighPass hpf80;
    FirstOrderHighPass hpf120;
    FirstOrderHighPass dcBlocker;
    CytomicSVF hfToneFilter;
    Oversampler4x oversampler;
};

} // namespace kinglion::dsp
