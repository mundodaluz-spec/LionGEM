#include "../Source/DSP/KingLionChannelStrip.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <iomanip>

using namespace kinglion::dsp;

static double measureSineRmsAfterSettling(KingLionChannelStrip& strip, double sampleRate, double freqHz, double ampLinear)
{
    constexpr double kTwoPi = 6.28318530717958647692;
    const int settleSamples = static_cast<int>(sampleRate * 0.35);
    const int measureSamples = static_cast<int>(sampleRate * 0.20);
    const int totalSamples = settleSamples + measureSamples;

    std::vector<float> bufL(static_cast<size_t>(totalSamples));
    std::vector<float> bufR(static_cast<size_t>(totalSamples));
    for (int n = 0; n < totalSamples; ++n)
    {
        const float s = static_cast<float>(ampLinear * std::sin(kTwoPi * freqHz * n / sampleRate));
        bufL[static_cast<size_t>(n)] = s;
        bufR[static_cast<size_t>(n)] = s;
    }

    strip.reset();
    for (int offset = 0; offset < totalSamples; offset += 256)
    {
        const int block = std::min(256, totalSamples - offset);
        strip.processBlock(bufL.data() + offset, bufR.data() + offset, block);
    }

    double sumSq = 0.0;
    for (int n = settleSamples; n < totalSamples; ++n)
    {
        const double v = bufL[static_cast<size_t>(n)];
        assert(std::isfinite(v));
        sumSq += v * v;
    }
    return std::sqrt(sumSq / static_cast<double>(measureSamples));
}

int main()
{
    std::cout << "=== Verificacao Matematica DSP: KingLion Box 1.0 (WYATT Sounds) ===\n";

    const double sr = 48000.0;
    KingLionChannelStrip strip;
    strip.prepare(sr);

    // 1. Teste de Bypass Geral (Power Off)
    {
        KingLionParameters p {};
        p.powerOn = false;
        strip.updateParameters(p);
        const double inAmp = 0.25;
        const double outRms = measureSineRmsAfterSettling(strip, sr, 1000.0, inAmp);
        const double expectedRms = inAmp / std::sqrt(2.0);
        const double diffDb = 20.0 * std::log10(outRms / expectedRms);
        std::cout << "[1] Power Bypass 1 kHz erro: " << std::fixed << std::setprecision(4) << diffDb << " dB\n";
        assert(std::abs(diffDb) < 0.01);
    }

    // 2. Teste do Low Cut suave de 6 dB/oitava (80 Hz e 120 Hz)
    {
        KingLionParameters p {};
        p.compEngaged = false;
        p.eqEngaged = false;
        p.deEssMode = 1; // Off
        p.xfmrEngaged = false;
        p.preampGainStep = 1; // 45 dB neutro
        p.lowCutMode = 0; // Flat
        strip.updateParameters(p);
        const double ref40Hz = measureSineRmsAfterSettling(strip, sr, 40.0, 0.1);

        p.lowCutMode = 2; // 120 Hz
        strip.updateParameters(p);
        const double cut40Hz = measureSineRmsAfterSettling(strip, sr, 40.0, 0.1);
        const double attenDb = 20.0 * std::log10(cut40Hz / ref40Hz);
        std::cout << "[2] Low Cut 120 Hz (6 dB/oct) @ 40 Hz: " << std::setprecision(2) << attenDb << " dB\n";
        // Em 40 Hz (fc/3 de 120 Hz), um filtro real de 1a ordem 6 dB/oct atenua -10.0 dB (1 / sqrt(1 + 9))
        assert(attenDb < -8.5 && attenDb > -11.5);
    }

    // 3. Teste do Compressor Optico Vactrol 3:1 Pre-Preamp
    {
        KingLionParameters p {};
        p.compEngaged = true;
        p.compThreshold = 7.5f;
        p.compAttackStep = 4;  // Fast (4 ms)
        p.compReleaseStep = 4; // Fast (0.3 s)
        p.eqEngaged = false;
        p.deEssMode = 1;
        p.xfmrEngaged = false;
        p.preampGainStep = 1;
        strip.updateParameters(p);

        const double outRms = measureSineRmsAfterSettling(strip, sr, 1000.0, 0.5);
        (void)outRms;
        const float grDb = strip.getCompGrDb();
        std::cout << "[3] Compressor Opto Vactrol 3:1 GR @ -6 dBFS (Thresh 7.5): -"
                  << std::setprecision(2) << grDb << " dB\n";
        assert(grDb > 4.0f && grDb <= 16.0f);
    }

    // 4. Teste da Chave GANHO Valvulado (40 dB vs 60 dB Negative Feedback)
    {
        KingLionParameters p {};
        p.compEngaged = false;
        p.eqEngaged = false;
        p.deEssMode = 1;
        p.xfmrEngaged = false;

        p.preampGainStep = 0; // 40 dB (alto feedback negativo)
        strip.updateParameters(p);
        const double rms40 = measureSineRmsAfterSettling(strip, sr, 440.0, 0.15);

        p.preampGainStep = 4; // 60 dB (baixo feedback negativo + harmonicos ricos)
        strip.updateParameters(p);
        const double rms60 = measureSineRmsAfterSettling(strip, sr, 440.0, 0.15);

        const double pushDb = 20.0 * std::log10(rms60 / rms40);
        std::cout << "[4] Preamp Classe A GANHO 40 dB -> 60 dB empuxo/densidade: +"
                  << std::setprecision(2) << pushDb << " dB\n";
        assert(pushDb > 4.0 && pushDb < 9.0);
    }

    // 5. Teste do EQ Passivo Pultec Estendido (Low +8 dB @ 100 Hz, Mid Dip -8 dB @ 1 kHz, High +8 dB @ 16 kHz)
    {
        KingLionParameters p {};
        p.compEngaged = false;
        p.preampGainStep = 1;
        p.deEssMode = 1;
        p.xfmrEngaged = false;
        p.eqEngaged = false;
        strip.updateParameters(p);

        const double flat100 = measureSineRmsAfterSettling(strip, sr, 100.0, 0.08);
        const double flat1k  = measureSineRmsAfterSettling(strip, sr, 1000.0, 0.08);
        const double flat16k = measureSineRmsAfterSettling(strip, sr, 16000.0, 0.08);

        p.eqEngaged = true;
        p.lowFreqStep = 4;   // 100 Hz
        p.lowBoostDb = 8.0f;
        p.midFreqStep = 4;   // 1 kHz
        p.midDipDb = 8.0f;
        p.highFreqStep = 9;  // 16 kHz
        p.highBoostDb = 8.0f;
        strip.updateParameters(p);

        const double eq100 = measureSineRmsAfterSettling(strip, sr, 100.0, 0.08);
        const double eq1k  = measureSineRmsAfterSettling(strip, sr, 1000.0, 0.08);
        const double eq16k = measureSineRmsAfterSettling(strip, sr, 16000.0, 0.08);

        const double d100 = 20.0 * std::log10(eq100 / flat100);
        const double d1k  = 20.0 * std::log10(eq1k  / flat1k);
        const double d16k = 20.0 * std::log10(eq16k / flat16k);

        std::cout << "[5] EQ Passivo Pultec 33 Bandas -> 100Hz: +" << d100
                  << " dB | 1kHz Dip: " << d1k
                  << " dB | 16kHz Air: +" << d16k << " dB\n";

        assert(d100 > 6.0 && d100 < 9.5);
        assert(d1k < -5.5 && d1k > -9.5);
        assert(d16k > 6.0 && d16k < 9.5);
    }

    // 6. Teste do De-Esser Passivo em 6 kHz vs Voz Fundamental em 250 Hz
    {
        KingLionParameters p {};
        p.compEngaged = false;
        p.preampGainStep = 1;
        p.eqEngaged = false;
        p.xfmrEngaged = false;
        p.deEssMode = 0;       // De-Esser ativo
        p.deEssFreqStep = 1;   // 6 kHz
        p.deEssThreshold = 7.5f;
        strip.updateParameters(p);

        const double out250 = measureSineRmsAfterSettling(strip, sr, 250.0, 0.35);
        const float grAt250 = strip.getDeEssGrDb();
        const double out6k  = measureSineRmsAfterSettling(strip, sr, 6000.0, 0.35);
        const float grAt6k  = strip.getDeEssGrDb();

        std::cout << "[6] De-Esser @ 6 kHz -> GR em 250 Hz: -" << grAt250
                  << " dB | GR em 6 kHz: -" << grAt6k
                  << " dB (seletividade: " << 20.0 * std::log10(out6k / out250) << " dB)\n";
        assert(grAt6k > grAt250 + 6.0f);
    }

    // 7. Estabilidade multi-sample-rate (44.1 kHz, 48 kHz, 96 kHz)
    for (double testSr : { 44100.0, 48000.0, 96000.0 })
    {
        strip.prepare(testSr);
        KingLionParameters p {};
        p.preampGainStep = 4;
        p.compThreshold = 6.0f;
        p.lowBoostDb = 6.0f;
        p.midDipDb = 4.0f;
        p.highFreqStep = 10; // 20 kHz!
        p.highBoostDb = 7.0f;
        p.deEssMode = 2;     // Limitador 10:1
        p.deEssThreshold = 6.0f;
        p.xfmrEngaged = true;
        strip.updateParameters(p);

        const double r = measureSineRmsAfterSettling(strip, testSr, 1000.0, 0.4);
        assert(std::isfinite(r) && r > 0.01 && r < 2.0);
    }
    std::cout << "[7] Estabilidade 44.1 / 48 / 96 kHz (incluindo banda de 20 kHz): APROVADO!\n";
    std::cout << "=== TODOS OS TESTES MATEMATICOS PASSARAM COM SUCESSO ===\n";
    return 0;
}
