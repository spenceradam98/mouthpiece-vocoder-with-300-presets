// Small DSP building blocks. Plain scalar C++ (no SIMD intrinsics, no -march flags).
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include <complex>
#include <algorithm>

namespace mp
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float flushDenormal (float v) { return std::fabs (v) < 1.0e-15f ? 0.0f : v; }

// One-pole coefficient for a time constant in ms.
inline float timeCoef (float ms, double fs)
{
    if (ms <= 0.0f) return 0.0f;
    return (float) std::exp (-1.0 / (0.001 * ms * fs));
}

struct Rng
{
    uint32_t s = 0x9e3779b9u;
    explicit Rng (uint32_t seed = 0x9e3779b9u) : s (seed ? seed : 1u) {}
    float white() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) ((double) s / 2147483648.0 - 1.0); }
};

// RBJ biquad, transposed direct form II.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void setBandpass (double fs, double fc, double q)   // 0 dB peak gain
    {
        fc = std::min (fc, fs * 0.45);
        const double w = kTwoPi * fc / fs, alpha = std::sin (w) / (2.0 * q), a0 = 1.0 + alpha;
        b0 = (float) (alpha / a0); b1 = 0; b2 = (float) (-alpha / a0);
        a1 = (float) (-2.0 * std::cos (w) / a0); a2 = (float) ((1.0 - alpha) / a0);
    }
    void setLowpass (double fs, double fc, double q = 0.7071)
    {
        fc = std::min (fc, fs * 0.45);
        const double w = kTwoPi * fc / fs, alpha = std::sin (w) / (2.0 * q), c = std::cos (w), a0 = 1.0 + alpha;
        b0 = (float) ((1.0 - c) * 0.5 / a0); b1 = (float) ((1.0 - c) / a0); b2 = b0;
        a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - alpha) / a0);
    }
    void setHighpass (double fs, double fc, double q = 0.7071)
    {
        fc = std::min (fc, fs * 0.45);
        const double w = kTwoPi * fc / fs, alpha = std::sin (w) / (2.0 * q), c = std::cos (w), a0 = 1.0 + alpha;
        b0 = (float) ((1.0 + c) * 0.5 / a0); b1 = (float) (-(1.0 + c) / a0); b2 = b0;
        a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - alpha) / a0);
    }
    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = flushDenormal (b1 * x - a1 * y + z2);
        z2 = flushDenormal (b2 * x - a2 * y);
        return y;
    }
    void reset() { z1 = z2 = 0; }
};

struct OnePoleLP
{
    float a = 0, z = 0;
    void set (double fs, double fc) { a = (float) std::exp (-kTwoPi * std::min (fc, fs * 0.45) / fs); }
    float process (float x) { z = flushDenormal (x + a * (z - x)); return z; }
};

// Peak-ish envelope follower with separate attack/release.
struct Follower
{
    float att = 0, rel = 0, env = 0;
    void set (double fs, float attackMs, float releaseMs) { att = timeCoef (attackMs, fs); rel = timeCoef (releaseMs, fs); }
    float process (float x)
    {
        const float a = std::fabs (x);
        const float c = a > env ? att : rel;
        env = flushDenormal (a + c * (env - a));
        return env;
    }
};

// In-place iterative radix-2 complex FFT.
class Fft
{
public:
    void init (int n)
    {
        size = n; levels = 0;
        while ((1 << levels) < n) ++levels;
        tw.resize ((size_t) n / 2);
        for (int i = 0; i < n / 2; ++i) tw[(size_t) i] = std::polar (1.0f, (float) (-kTwoPi * i / n));
        rev.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            int r = 0;
            for (int b = 0; b < levels; ++b) r |= ((i >> b) & 1) << (levels - 1 - b);
            rev[(size_t) i] = r;
        }
    }
    int getSize() const { return size; }

    void transform (std::complex<float>* x, bool inverse) const
    {
        const int n = size;
        for (int i = 0; i < n; ++i)
            if (rev[(size_t) i] > i) std::swap (x[i], x[rev[(size_t) i]]);
        for (int len = 2; len <= n; len <<= 1)
        {
            const int half = len / 2, step = n / len;
            for (int i = 0; i < n; i += len)
                for (int j = 0; j < half; ++j)
                {
                    auto w = tw[(size_t) (j * step)];
                    if (inverse) w = std::conj (w);
                    const auto u = x[i + j], v = x[i + j + half] * w;
                    x[i + j] = u + v;
                    x[i + j + half] = u - v;
                }
        }
        if (inverse)
        {
            const float s = 1.0f / (float) n;
            for (int i = 0; i < n; ++i) x[i] *= s;
        }
    }

private:
    int size = 0, levels = 0;
    std::vector<std::complex<float>> tw;
    std::vector<int> rev;
};

// PolyBLEP residual for band-limited discontinuities. t = phase (0..1), dt = phase increment.
inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// Simple modulated delay line used by the ensemble.
struct DelayLine
{
    std::vector<float> buf;
    int w = 0;
    void init (int len) { buf.assign ((size_t) std::max (len, 4), 0.0f); w = 0; }
    void push (float x) { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
    float readFrac (float delay) const   // delay in samples, >= 1
    {
        const int n = (int) buf.size();
        float pos = (float) w - delay;
        while (pos < 0) pos += (float) n;
        const int i0 = (int) pos;
        const float f = pos - (float) i0;
        const float a = buf[(size_t) (i0 % n)], b = buf[(size_t) ((i0 + 1) % n)];
        return a + f * (b - a);
    }
    float read (int delay) const
    {
        const int n = (int) buf.size();
        int i = w - delay;
        while (i < 0) i += n;
        return buf[(size_t) i];
    }
};
} // namespace mp
