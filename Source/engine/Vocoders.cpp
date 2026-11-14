// The four vocoder cores plus the post effects.
#include "Engine.h"

namespace mp
{
// ============================================================================ BankVocoder (Classic / Vintage)
void BankVocoder::prepare (double sampleRate)
{
    fs = sampleRate;
    cfgBands = -1;
    reset();
}

void BankVocoder::reset()
{
    for (int i = 0; i < kMaxBands; ++i)
    {
        anaA[(size_t) i].reset(); anaB[(size_t) i].reset(); synA[(size_t) i].reset(); synB[(size_t) i].reset();
        env[(size_t) i] = envSmooth[(size_t) i] = held[(size_t) i] = 0;
    }
}

void BankVocoder::configure (int nb, float lowHz, float highHz, float formantSemis, float bandwidth, float attackMs, float releaseMs,
                             bool isVintage, float robot, bool frz)
{
    nb = std::max (2, std::min (kMaxBands, nb));
    freeze = frz;
    holdInc = robot > 0.001f ? (float) ((40.0 * (1.0 - robot) + 3.0) / fs) : 0.0f;

    if (attackMs != cfgAtt || releaseMs != cfgRel || nb != cfgBands || isVintage != cfgVintage)
        for (int i = 0; i < nb; ++i)
        {
            // low bands get slightly slower followers so they don't ripple at the band's own frequency
            const float slow = 1.0f + 1.5f * (1.0f - (float) i / (float) std::max (1, nb - 1));
            attC[(size_t) i] = timeCoef (attackMs * slow, fs);
            relC[(size_t) i] = timeCoef (releaseMs * (isVintage ? 1.2f : 1.0f), fs);
        }
    cfgAtt = attackMs; cfgRel = releaseMs;

    if (nb == cfgBands && lowHz == cfgLow && highHz == cfgHigh && formantSemis == cfgFormant && bandwidth == cfgBw && isVintage == cfgVintage)
        return;

    const bool layoutChanged = nb != cfgBands || isVintage != cfgVintage;
    bands = nb; vintage = isVintage;
    cfgBands = nb; cfgLow = lowHz; cfgHigh = highHz; cfgFormant = formantSemis; cfgBw = bandwidth; cfgVintage = isVintage;

    const double lo = std::max (20.0, (double) std::min (lowHz, highHz * 0.5f));
    const double hi = std::min ((double) highHz, fs * 0.45);
    const double step = std::pow (hi / lo, 1.0 / (nb - 1));
    const double bwOct = std::log2 (step) * (0.55 + 1.6 * bandwidth) * (isVintage ? 1.25 : 1.0);
    const double q = 1.0 / (2.0 * std::sinh (std::log (2.0) * 0.5 * bwOct));
    // a 4th-order bandpass (two stages) is narrower than one stage with the same q, so open the stages up
    const double stageQ = isVintage ? q : q * 0.64;
    const double ratio = std::pow (2.0, formantSemis / 12.0);

    for (int i = 0; i < nb; ++i)
    {
        const double fa = lo * std::pow (step, i);
        const double fsyn = fa * ratio;
        anaA[(size_t) i].setBandpass (fs, fa, stageQ);
        anaB[(size_t) i].setBandpass (fs, fa, stageQ);
        synA[(size_t) i].setBandpass (fs, fsyn, stageQ);
        synB[(size_t) i].setBandpass (fs, fsyn, stageQ);
        const bool audible = fsyn < fs * 0.44 && fsyn > 20.0;
        const double trimDb = isVintage ? (double) rng.white() * 1.2 : 0.0;   // component tolerance on the vintage bank
        trim[(size_t) i] = audible ? (float) std::pow (10.0, trimDb / 20.0) : 0.0f;
        // level compensation: narrower bands pass less energy; the sqrt(q) term keeps loudness steady across band counts
        makeup[(size_t) i] = (float) (std::sqrt (q) * (isVintage ? 1.3 : 1.8) * std::pow (fa / 1000.0, 0.12));
        const bool odd = (i & 1) != 0;
        panL[(size_t) i] = odd ? 0.0f : 1.0f;
        panR[(size_t) i] = odd ? 1.0f : 0.0f;
    }
    if (layoutChanged) reset();
    gain = (isVintage ? 11.0f * std::pow ((float) nb / 20.0f, 0.15f) : 34.0f * std::pow ((float) nb / 20.0f, 0.7f)) / std::sqrt ((float) nb);
}

void BankVocoder::process (float m, float c, float spread, float& outL, float& outR)
{
    bool sample = false;
    if (holdInc > 0)
    {
        holdPhase += holdInc;
        if (holdPhase >= 1.0f) { holdPhase -= 1.0f; sample = true; }
    }
    const float side = clampf (spread, 0.0f, 1.0f);
    float l = 0, r = 0;
    for (int i = 0; i < bands; ++i)
    {
        const size_t b = (size_t) i;
        float a = anaA[b].process (m);
        if (! vintage) a = anaB[b].process (a);
        if (! freeze)
        {
            const float x = std::fabs (a);
            const float coef = x > env[b] ? attC[b] : relC[b];
            env[b] = flushDenormal (x + coef * (env[b] - x));
        }
        float e = env[b];
        if (vintage)
        {
            envSmooth[b] = flushDenormal (e + relC[b] * 0.5f * (envSmooth[b] - e));   // second detector pole: softer, slower
            e = envSmooth[b];
        }
        if (holdInc > 0) { if (sample) held[b] = e; e = held[b]; }

        float cs = synA[b].process (c);
        if (! vintage) cs = synB[b].process (cs);
        const float y = cs * e * makeup[b] * trim[b];
        // pan: centre at spread 0, alternate hard left/right at spread 1
        const float pl = 1.0f - side * (1.0f - panL[b]), pr = 1.0f - side * (1.0f - panR[b]);
        l += y * pl; r += y * pr;
    }
    l *= gain; r *= gain;
    if (vintage)
    {
        l = std::tanh (l * 1.4f) / 1.4f;
        r = std::tanh (r * 1.4f) / 1.4f;
    }
    outL = l; outR = r;
}

// ============================================================================ SpectralVocoder
void SpectralVocoder::prepare (double sampleRate)
{
    fs = sampleRate;
    n = fs > 60000.0 ? 2048 : 1024;
    hop = n / 4;
    fft.init (n);
    window.resize ((size_t) n);
    for (int i = 0; i < n; ++i) window[(size_t) i] = (float) std::sqrt (0.5 - 0.5 * std::cos (kTwoPi * i / n));
    const size_t bins = (size_t) n / 2 + 1;
    inM.assign ((size_t) n, 0); inC.assign ((size_t) n, 0); outL.assign ((size_t) n, 0); outR.assign ((size_t) n, 0);
    envM.assign (bins, 0); envMs.assign (bins, 0); magM.assign (bins, 0); magC.assign (bins, 0);
    prefix.assign (bins + 1, 0); envC.assign (bins, 0); tmp.assign (bins, 0);
    bufM.assign ((size_t) n, {}); bufC.assign ((size_t) n, {}); specL.assign ((size_t) n, {}); specR.assign ((size_t) n, {});
    reset();
}

void SpectralVocoder::reset()
{
    std::fill (inM.begin(), inM.end(), 0.0f); std::fill (inC.begin(), inC.end(), 0.0f);
    std::fill (outL.begin(), outL.end(), 0.0f); std::fill (outR.begin(), outR.end(), 0.0f);
    std::fill (envMs.begin(), envMs.end(), 0.0f); std::fill (tmp.begin(), tmp.end(), 0.0f);
    pos = 0; hopCount = 0;
}

void SpectralVocoder::configure (int nb, float lowHz, float highHz, float formantSemis, float bw, float attackMs, float releaseMs,
                                 float rob, bool frz)
{
    bands = nb; low = lowHz; high = highHz; bandwidth = bw;
    ratio = std::pow (2.0f, formantSemis / 12.0f);
    attC = (float) std::exp (-(double) hop / (0.001 * std::max (0.5f, attackMs) * fs));
    relC = (float) std::exp (-(double) hop / (0.001 * std::max (5.0f, releaseMs) * fs));
    robot = rob; freeze = frz;
}

void SpectralVocoder::process (float m, float c, float spread, float& l, float& r)
{
    inM[(size_t) pos] = m;
    inC[(size_t) pos] = c;
    l = outL[(size_t) pos]; r = outR[(size_t) pos];
    outL[(size_t) pos] = 0; outR[(size_t) pos] = 0;
    if (++pos >= n) pos = 0;
    if (++hopCount >= hop) { hopCount = 0; frame (spread); }
}

namespace
{
// constant-Q smoothing of a magnitude spectrum: average over k*(1 +/- q) bins
void smoothSpectrum (const std::vector<float>& mag, std::vector<float>& prefix, std::vector<float>& out, float q, int minW)
{
    const int bins = (int) mag.size();
    prefix[0] = 0;
    for (int k = 0; k < bins; ++k) prefix[(size_t) k + 1] = prefix[(size_t) k] + mag[(size_t) k];
    for (int k = 0; k < bins; ++k)
    {
        const int w = std::max (minW, (int) (k * q + 0.5f));
        const int a = std::max (0, k - w), b = std::min (bins - 1, k + w);
        out[(size_t) k] = (prefix[(size_t) b + 1] - prefix[(size_t) a]) / (float) (b - a + 1);
    }
}
}

void SpectralVocoder::frame (float spread)
{
    // one complex FFT carries both real inputs: x = m + i*c
    for (int j = 0; j < n; ++j)
    {
        const size_t idx = (size_t) ((pos + j) % n);
        bufM[(size_t) j] = { inM[idx] * window[(size_t) j], inC[idx] * window[(size_t) j] };
    }
    fft.transform (bufM.data(), false);
    const int half = n / 2;
    for (int k = 0; k <= half; ++k)
    {
        const auto xk = bufM[(size_t) k], xn = std::conj (bufM[(size_t) ((n - k) % n)]);
        const auto mk = (xk + xn) * 0.5f;
        const auto ck = (xk - xn) * std::complex<float> (0.0f, -0.5f);
        bufC[(size_t) k] = ck;
        magM[(size_t) k] = std::abs (mk);
        magC[(size_t) k] = std::abs (ck);
    }

    // envelopes: band count sets the modulator resolution; the carrier is flattened with a wide smoother
    const float qM = (1.6f / (float) std::max (4, bands)) * (0.45f + 1.1f * bandwidth);
    smoothSpectrum (magM, prefix, envM, qM, 1);
    smoothSpectrum (magC, prefix, envC, 0.3f, 3);

    // time smoothing, freeze and robot sample-and-hold
    bool take = true;
    if (robot > 0.001f)
    {
        holdPhase += (float) ((40.0 * (1.0 - robot) + 3.0) * hop / fs);
        take = holdPhase >= 1.0f;
        if (take) holdPhase -= std::floor (holdPhase);
    }
    if (! freeze)
        for (int k = 0; k <= half; ++k)
        {
            const float x = envM[(size_t) k], cur = envMs[(size_t) k];
            const float coef = x > cur ? attC : relC;
            envMs[(size_t) k] = flushDenormal (x + coef * (cur - x));
        }
    if (robot <= 0.001f) std::copy (envMs.begin(), envMs.end(), tmp.begin());
    else if (take) std::copy (envMs.begin(), envMs.end(), tmp.begin());

    float cMax = 1.0e-9f;
    for (int k = 0; k <= half; ++k) cMax = std::max (cMax, envC[(size_t) k]);
    const float eps = cMax * 0.003f + 1.0e-7f;

    const float binHz = (float) (fs / n);
    const float side = clampf (spread, 0.0f, 1.0f);
    for (int k = 0; k <= half; ++k)
    {
        const float f = k * binHz;
        // formant shift: the output at f takes the modulator envelope from f / ratio
        const float src = (float) k / ratio;
        const int i0 = (int) src;
        float e = 0;
        if (i0 < half)
        {
            const float fr = src - (float) i0;
            e = tmp[(size_t) i0] + fr * (tmp[(size_t) i0 + 1] - tmp[(size_t) i0]);
        }
        // soft band limits
        const float fsrc = src * binHz;
        float range = 1.0f;
        if (fsrc < low) range = clampf ((fsrc - low * 0.6f) / (low * 0.4f), 0.0f, 1.0f);
        else if (fsrc > high) range = clampf ((high * 1.4f - fsrc) / (high * 0.4f), 0.0f, 1.0f);

        const auto y = bufC[(size_t) k] / (envC[(size_t) k] + eps) * (e * range * 1.45f);
        const int band = f > low ? (int) (std::log2 (f / low) * 2.0f) : 0;
        const bool odd = (band & 1) != 0;
        const float gl = odd ? 1.0f - side : 1.0f, gr = odd ? 1.0f : 1.0f - side;
        specL[(size_t) k] = y * gl;
        specR[(size_t) k] = y * gr;
    }
    // pack two Hermitian spectra into one complex IFFT: z = L + i*R
    for (int k = 0; k <= half; ++k)
    {
        const auto lk = specL[(size_t) k], rk = specR[(size_t) k];
        bufM[(size_t) k] = lk + std::complex<float> (0, 1) * rk;
        if (k > 0 && k < half) bufM[(size_t) (n - k)] = std::conj (lk) + std::complex<float> (0, 1) * std::conj (rk);
    }
    bufM[0] = { specL[0].real(), specR[0].real() };
    bufM[(size_t) half] = { specL[(size_t) half].real(), specR[(size_t) half].real() };
    fft.transform (bufM.data(), true);

    // overlap-add (sqrt-Hann analysis * synthesis at 75% overlap sums to 2)
    for (int j = 0; j < n; ++j)
    {
        const size_t idx = (size_t) ((pos + j) % n);
        const float wj = window[(size_t) j] * 0.5f;
        outL[idx] += bufM[(size_t) j].real() * wj;
        outR[idx] += bufM[(size_t) j].imag() * wj;
    }
}

void SpectralVocoder::displayLevels (float* out, int count, float lowHz, float highHz) const
{
    const float binHz = (float) (fs / n);
    for (int b = 0; b < count; ++b)
    {
        const float f0 = lowHz * std::pow (highHz / lowHz, (float) b / count), f1 = lowHz * std::pow (highHz / lowHz, (float) (b + 1) / count);
        int k0 = std::max (1, (int) (f0 / binHz)), k1 = std::max (k0 + 1, (int) (f1 / binHz));
        k1 = std::min (k1, n / 2);
        float s = 0;
        for (int k = k0; k < k1; ++k) s += tmp[(size_t) k];
        out[b] = k1 > k0 ? s / (float) (k1 - k0) * 8.0f / (float) n * 64.0f : 0.0f;
    }
}

// ============================================================================ LpcVocoder (Talkbox)
void LpcVocoder::prepare (double sampleRate)
{
    fs = sampleRate;
    order = std::max (16, std::min (kMaxOrder, (int) std::lround (fs / 1800.0) + 2));
    win = (int) std::lround (0.03 * fs);
    hop = win / 4;
    ring.assign ((size_t) win * 3 + 8, 0.0f);
    window.resize ((size_t) win);
    for (int i = 0; i < win; ++i) window[(size_t) i] = (float) (0.5 - 0.5 * std::cos (kTwoPi * (i + 0.5) / win));
    frameBuf.assign ((size_t) win, 0.0f);
    reset();
}

void LpcVocoder::reset()
{
    std::fill (ring.begin(), ring.end(), 0.0f);
    kOld.fill (0); kNew.fill (0); kCur.fill (0); bState.fill (0); aDisplay.fill (0);
    gOld = gNew = 0; interp = 1; preZ = deZ = 0; cPow = 0.01f; w = 0; hopCount = 0; gDisplay = 0;
}

void LpcVocoder::configure (float formantSemis, float bandwidth, float emph, float rob, bool frz)
{
    ratio = std::pow (2.0f, clampf (formantSemis, -12.0f, 12.0f) / 12.0f);
    bwHz = 20.0f + bandwidth * 220.0f;
    emphasis = emph; robot = rob; freeze = frz;
}

void LpcVocoder::analyse()
{
    const int len = (int) ring.size();
    // read the newest window; a ratio > 1 reads a longer stretch faster, which raises the formants
    const float span = (float) (win - 1) * ratio;
    for (int j = 0; j < win; ++j)
    {
        const float back = span - (float) j * ratio;   // samples before the newest
        float p = (float) (w - 1) - back;
        while (p < 0) p += (float) len;
        const int i0 = (int) p;
        const float fr = p - (float) i0;
        const float a0 = ring[(size_t) (i0 % len)], a1 = ring[(size_t) ((i0 + 1) % len)];
        frameBuf[(size_t) j] = (a0 + fr * (a1 - a0)) * window[(size_t) j];
    }
    for (int lag = 0; lag <= order; ++lag)
    {
        double s = 0;
        for (int j = lag; j < win; ++j) s += (double) frameBuf[(size_t) j] * frameBuf[(size_t) (j - lag)];
        const double g = (kTwoPi * bwHz * lag / fs);
        r[(size_t) lag] = s * std::exp (-0.5 * g * g);
    }
    double winPow = 0;
    for (int j = 0; j < win; ++j) winPow += (double) window[(size_t) j] * window[(size_t) j];

    for (int i = 0; i <= order; ++i) kOld[(size_t) i] = kCur[(size_t) i];
    gOld = gOld + (gNew - gOld) * std::min (1.0f, interp);
    interp = 0;
    interpStep = 1.0f / (float) hop;

    if (r[0] < 1.0e-9 * win)
    {
        gNew = 0;   // silence: keep the filter shape, fade the excitation
        return;
    }
    r[0] *= 1.0001;
    a.fill (0); a[0] = 1;
    double err = r[0];
    for (int i = 1; i <= order; ++i)
    {
        double acc = r[(size_t) i];
        for (int j = 1; j < i; ++j) acc += a[(size_t) j] * r[(size_t) (i - j)];
        double k = -acc / err;
        k = std::max (-0.995, std::min (0.995, k));
        for (int j = 1; j < i; ++j) tmpA[(size_t) j] = a[(size_t) j] + k * a[(size_t) (i - j)];
        for (int j = 1; j < i; ++j) a[(size_t) j] = tmpA[(size_t) j];
        a[(size_t) i] = k;
        kNew[(size_t) i] = (float) k;
        err *= (1.0 - k * k);
        if (err <= 0) { err = 1.0e-12; }
    }
    gNew = (float) std::sqrt (err / std::max (1.0, winPow));
    for (int i = 0; i <= order; ++i) aDisplay[(size_t) i] = (float) a[(size_t) i];
    gDisplay = gNew;
}

float LpcVocoder::process (float m, float c)
{
    // analysis path: pre-emphasis flattens the voice's natural tilt so the predictor spends its order on formants
    const float pe = m - 0.95f * preZ;
    preZ = m;
    ring[(size_t) w] = pe;
    if (++w >= (int) ring.size()) w = 0;
    if (++hopCount >= hop)
    {
        hopCount = 0;
        bool take = ! freeze;
        if (take && robot > 0.001f)
        {
            holdPhase += (float) ((40.0 * (1.0 - robot) + 3.0) * hop / fs);
            take = holdPhase >= 1.0f;
            if (take) holdPhase -= std::floor (holdPhase);
        }
        if (take) analyse();
    }

    if (interp < 1.0f) interp = std::min (1.0f, interp + interpStep);
    for (int i = 1; i <= order; ++i) kCur[(size_t) i] = kOld[(size_t) i] + (kNew[(size_t) i] - kOld[(size_t) i]) * interp;
    const float g = gOld + (gNew - gOld) * interp;

    // carrier normalised to unit power so the voice alone sets the level
    cPow = flushDenormal (c * c + 0.9995f * (cPow - c * c));
    const float x = c / std::sqrt (cPow + 1.0e-6f) * g;

    float f = x;
    for (int i = order; i >= 1; --i)
    {
        f -= kCur[(size_t) i] * bState[(size_t) i - 1];
        bState[(size_t) i] = flushDenormal (bState[(size_t) i - 1] + kCur[(size_t) i] * f);
    }
    bState[0] = f;
    // de-emphasis: Emphasis keeps more of the bright, pre-emphasised talkbox edge
    const float de = 0.95f * (1.0f - emphasis * 0.75f);
    deZ = flushDenormal (f + de * deZ);
    const float y = deZ * (1.0f - de) * 4.5f + f * 0.27f;
    return clampf (y, -4.0f, 4.0f);
}

void LpcVocoder::displayLevels (float* out, int count, float lowHz, float highHz) const
{
    for (int b = 0; b < count; ++b)
    {
        const double f = lowHz * std::pow (highHz / lowHz, (b + 0.5) / count);
        const double wv = kTwoPi * f / fs;
        double re = 0, im = 0;
        for (int i = 0; i <= order; ++i) { re += aDisplay[(size_t) i] * std::cos (wv * i); im -= aDisplay[(size_t) i] * std::sin (wv * i); }
        const double mag = std::sqrt (re * re + im * im);
        out[b] = mag > 1.0e-9 ? (float) (gDisplay / mag) * 2.0f : 0.0f;
    }
}

// ============================================================================ Ensemble
void Ensemble::prepare (double sampleRate)
{
    fs = sampleRate;
    dl.init ((int) (0.05 * fs));
    dr.init ((int) (0.05 * fs));
}

void Ensemble::process (float& l, float& r, float amount)
{
    dl.push (l); dr.push (r);
    if (amount < 0.001f) return;
    static const float rates[3] = { 0.43f, 0.61f, 0.79f };
    float wl = 0, wr = 0;
    for (int i = 0; i < 3; ++i)
    {
        ph[i] += (float) (rates[i] / fs);
        if (ph[i] >= 1.0f) ph[i] -= 1.0f;
        const float sl = (float) std::sin (kTwoPi * ph[i]), sr = (float) std::sin (kTwoPi * (ph[i] + 0.5f));
        const float base = (float) (0.008 * fs), depth = (float) (0.0035 * fs) * (0.4f + 0.6f * amount);
        wl += dl.readFrac (base + depth * (0.5f + 0.5f * sl));
        wr += dr.readFrac (base + depth * (0.5f + 0.5f * sr));
    }
    l = l * (1.0f - 0.45f * amount) + wl * 0.33f * amount;
    r = r * (1.0f - 0.45f * amount) + wr * 0.33f * amount;
}

// ============================================================================ Space (8-line FDN)
void Space::prepare (double fs)
{
    static const double times[kLines] = { 0.0297, 0.0371, 0.0411, 0.0437, 0.0533, 0.0587, 0.0631, 0.0713 };
    for (int i = 0; i < kLines; ++i)
    {
        len[i] = (int) (times[i] * fs);
        lines[i].init (len[i] + 2);
        lp[i] = 0;
    }
}

void Space::process (float& l, float& r, float amount)
{
    float o[kLines];
    float sum = 0;
    for (int i = 0; i < kLines; ++i) { o[i] = lines[i].read (len[i]); sum += o[i]; }
    if (amount < 0.001f && std::fabs (sum) < 1.0e-9f)
    {
        for (int i = 0; i < kLines; ++i) lines[i].push (0.0f);
        return;
    }
    const float fb = 0.80f + 0.17f * amount;
    const float damp = 0.35f;
    const float in = (l + r) * 0.5f;
    const float house = sum * (2.0f / kLines);
    float wl = 0, wr = 0;
    for (int i = 0; i < kLines; ++i)
    {
        float v = (o[i] - house) * fb;
        lp[i] = flushDenormal (v + damp * (lp[i] - v));
        lines[i].push (lp[i] + in * ((i & 1) ? 0.35f : -0.35f));
        if (i & 1) wr += o[i]; else wl += o[i];
    }
    const float wet = amount * 0.55f;
    l = l * (1.0f - amount * 0.35f) + wl * wet;
    r = r * (1.0f - amount * 0.35f) + wr * wet;
}
} // namespace mp
