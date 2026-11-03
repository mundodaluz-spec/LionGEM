#pragma once
#include <cmath>
#include <algorithm>

namespace kinglion::dsp
{

/**
 * Zero-Delay Feedback (Trapezoidal) State Variable Filter (Andrew Simper / Cytomic).
 * Mantem curvas abertas e musicais ate 20 kHz sem o "cramping" de fase/frequencia
 * tipico de biquads digitais comuns perto de Nyquist.
 */
class CytomicSVF
{
public:
    enum class Type
    {
        Bell,
        LowShelf,
        HighShelf,
        BandPass,
        Notch,
        HighPass
    };

    void reset() noexcept
    {
        ic1eq = 0.0;
        ic2eq = 0.0;
    }

    void setParameters(Type type, double sampleRate, double freqHz, double q, double gainDb) noexcept
    {
        currentType = type;
        const double nyquistSafe = std::clamp(freqHz, 10.0, sampleRate * 0.475);
        constexpr double kPi = 3.14159265358979323846;

        A = std::pow(10.0, gainDb / 40.0);
        g = std::tan(kPi * nyquistSafe / sampleRate);
        k = 1.0 / std::max(0.05, q);

        switch (type)
        {
            case Type::Bell:
                k = 1.0 / (std::max(0.05, q) * A);
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
                m0 = 1.0;
                m1 = k * (A * A - 1.0);
                m2 = 0.0;
                break;

            case Type::LowShelf:
                g /= std::sqrt(A);
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
                m0 = 1.0;
                m1 = k * (A - 1.0);
                m2 = A * A - 1.0;
                break;

            case Type::HighShelf:
                g *= std::sqrt(A);
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
                m0 = A * A;
                m1 = k * (1.0 - A) * A;
                m2 = 1.0 - A * A;
                break;

            case Type::BandPass:
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
                m0 = 0.0;
                m1 = 1.0;
                m2 = 0.0;
                break;

            case Type::Notch:
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
                m0 = 1.0;
                m1 = -k;
                m2 = 0.0;
                break;

            case Type::HighPass:
                a1 = 1.0 / (1.0 + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
                m0 = 1.0;
                m1 = -k;
                m2 = -1.0;
                break;
        }
    }

    inline double processSample(double v0) noexcept
    {
        const double v3 = v0 - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;
        return m0 * v0 + m1 * v1 + m2 * v2;
    }

private:
    Type currentType = Type::Bell;
    double ic1eq = 0.0, ic2eq = 0.0;
    double A = 1.0, g = 0.0, k = 1.0;
    double a1 = 0.0, a2 = 0.0, a3 = 0.0;
    double m0 = 1.0, m1 = 0.0, m2 = 0.0;
};

/**
 * Filtro Passa-Altas Trapezoidal de 1a Ordem (6 dB/oitava) — replica o capacitor
 * de acoplamento suave do Low Cut da VOXBOX (Flat / 80 Hz / 120 Hz) preservando a fase.
 */
class FirstOrderHighPass
{
public:
    void reset() noexcept { s1 = 0.0; }

    void setCutoff(double sampleRate, double cutoffHz) noexcept
    {
        constexpr double kPi = 3.14159265358979323846;
        const double fc = std::clamp(cutoffHz, 1.0, sampleRate * 0.45);
        const double g = std::tan(kPi * fc / sampleRate);
        G = g / (1.0 + g);
    }

    inline double processSample(double x) noexcept
    {
        const double v = (x - s1) * (1.0 - G);
        const double lp = x - v;
        s1 = lp + G * (x - s1);
        return v;
    }

private:
    double G = 0.0;
    double s1 = 0.0;
};

} // namespace kinglion::dsp
