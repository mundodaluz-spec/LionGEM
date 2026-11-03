#pragma once
#include <array>
#include <cmath>

namespace kinglion::dsp
{

/**
 * Oversampler Polifasico Allpass Half-Band 4x (2 estagios 2x em cascata).
 * Coeficientes elipticos de alta rejeicao (>85 dB stopband) com baixissima carga
 * de CPU, evitando aliasing digital nas harmonicas de valvula e transformador.
 */
class HalfBandStage2x
{
public:
    void reset() noexcept
    {
        upA.fill({});
        upB.fill({});
        dnA.fill({});
        dnB.fill({});
        dnPrevB = 0.0;
    }

    inline void upsample(double input, double& out0, double& out1) noexcept
    {
        double a = input;
        double b = input;
        for (size_t i = 0; i < kNumCoeffs; ++i)
        {
            a = processAllpass(a, kCoeffsA[i], upA[i]);
            b = processAllpass(b, kCoeffsB[i], upB[i]);
        }
        out0 = a;
        out1 = upDelayB;
        upDelayB = b;
    }

    inline double downsample(double in0, double in1) noexcept
    {
        double a = in0;
        double b = dnPrevB;
        dnPrevB = in1;

        for (size_t i = 0; i < kNumCoeffs; ++i)
        {
            a = processAllpass(a, kCoeffsA[i], dnA[i]);
            b = processAllpass(b, kCoeffsB[i], dnB[i]);
        }
        return 0.5 * (a + b);
    }

private:
    struct AllpassState
    {
        double x1 = 0.0;
        double y1 = 0.0;
    };

    static inline double processAllpass(double x, double c, AllpassState& st) noexcept
    {
        const double y = c * (x + st.y1) - st.x1;
        st.x1 = x;
        st.y1 = y;
        return y;
    }

    static constexpr size_t kNumCoeffs = 4;
    static constexpr std::array<double, kNumCoeffs> kCoeffsA {
        0.036681502163648017, 0.27463175937945444, 0.56109869787919531, 0.76974183386322703
    };
    static constexpr std::array<double, kNumCoeffs> kCoeffsB {
        0.13654762463195794, 0.42313861743656711, 0.67754004997416184, 0.88962489098817857
    };

    std::array<AllpassState, kNumCoeffs> upA {}, upB {}, dnA {}, dnB {};
    double upDelayB = 0.0;
    double dnPrevB = 0.0;
};

class Oversampler4x
{
public:
    void reset() noexcept
    {
        stage1.reset();
        stage2.reset();
    }

    template <typename NonlinearFn>
    inline double processSample(double x, NonlinearFn&& fn) noexcept
    {
        double s1_0 = 0.0, s1_1 = 0.0;
        stage1.upsample(x, s1_0, s1_1);

        double u0 = 0.0, u1 = 0.0, u2 = 0.0, u3 = 0.0;
        stage2.upsample(s1_0, u0, u1);
        stage2.upsample(s1_1, u2, u3);

        u0 = fn(u0);
        u1 = fn(u1);
        u2 = fn(u2);
        u3 = fn(u3);

        const double d0 = stage2.downsample(u0, u1);
        const double d1 = stage2.downsample(u2, u3);
        return stage1.downsample(d0, d1);
    }

private:
    HalfBandStage2x stage1;
    HalfBandStage2x stage2;
};

} // namespace kinglion::dsp
