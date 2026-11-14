// Carrier synth and pitch tracker.
#include "Engine.h"

namespace mp
{
// ============================================================================ Synth
void Synth::prepare (double sampleRate)
{
    fs = sampleRate;
    allOff (true);
}

void Synth::noteOn (int tag, float note, float velocity)
{
    // reuse a voice already playing this tag+note (retrigger), else a free voice, else steal the oldest
    Voice* v = nullptr;
    for (auto& vc : voices)
        if (vc.active && vc.tag == tag && std::fabs (vc.target - note) < 0.01f) { v = &vc; break; }
    if (v == nullptr)
        for (auto& vc : voices)
            if (! vc.active) { v = &vc; break; }
    if (v == nullptr)
    {
        uint32_t oldest = UINT32_MAX;
        for (auto& vc : voices)
        {
            const uint32_t score = vc.age + (vc.releasing ? 0u : 0x40000000u);   // prefer releasing voices
            if (score < oldest) { oldest = score; v = &vc; }
        }
    }

    // glide starts from the newest sounding voice's pitch
    float from = note;
    if (set.glideMs > 0.5f)
    {
        uint32_t newest = 0;
        for (auto& vc : voices)
            if (vc.active && &vc != v && vc.age >= newest) { newest = vc.age; from = vc.current; }
    }

    const bool wasActive = v->active;
    v->active = true;
    v->releasing = false;
    v->tag = tag;
    v->age = ++counter;
    v->target = note;
    v->current = wasActive ? v->current : from;
    v->vel = velocity;
    if (! wasActive)
    {
        v->env = 0;
        for (int u = 0; u < kMaxUnison; ++u) v->phase[u] = rng.white() * 0.5f + 0.5f;   // free-running feel, avoids phasey unison
        v->phase2 = rng.white() * 0.5f + 0.5f;
        v->subPhase = 0;
    }
}

void Synth::noteOff (int tag)
{
    for (auto& v : voices)
        if (v.active && v.tag == tag) v.releasing = true;
}

void Synth::allOff (bool hard)
{
    for (auto& v : voices)
    {
        if (hard) { v.active = false; v.env = 0; }
        v.releasing = true;
    }
}

int Synth::activeVoices() const
{
    int n = 0;
    for (auto& v : voices) n += v.active && ! v.releasing ? 1 : 0;
    return n;
}

float Synth::osc (int wave, float& phase, float dt, float pw)
{
    const float t = phase;
    phase += dt;
    if (phase >= 1.0f) phase -= 1.0f;

    switch (wave)
    {
        case WSaw: return 2.0f * t - 1.0f - polyBlep (t, dt);
        case WSquare:
        case WPulse:
        {
            const float width = wave == WSquare ? 0.5f : pw;
            float v = t < width ? 1.0f : -1.0f;
            v += polyBlep (t, dt);
            float t2 = t + (1.0f - width);
            if (t2 >= 1.0f) t2 -= 1.0f;
            v -= polyBlep (t2, dt);
            return v - (2.0f * width - 1.0f);   // remove DC of asymmetric pulses
        }
        case WTriangle: return 4.0f * std::fabs (t - 0.5f) - 1.0f;
        case WSine: return (float) std::sin (kTwoPi * t);
        case WBuzz:
        default:
        {
            // band-limited impulse train (closed-form sum of harmonics up to Nyquist)
            const int m = std::max (1, std::min (200, (int) (0.5f / std::max (dt, 1.0e-6f))));
            const double x = kPi * t, den = std::sin (x);
            const double k = 2.0 * m + 1.0;
            const double v = std::fabs (den) < 1.0e-6 ? 1.0 : std::sin (k * x) / (k * den);
            return (float) ((v - 1.0 / k) * 4.0);
        }
    }
}

float Synth::process()
{
    const float glideC = timeCoef (set.glideMs * 0.5f, fs);
    const float attC = timeCoef (std::max (0.5f, set.attackMs) * 0.4f, fs);   // 0.4: reach ~92% in the stated time
    const float relC = timeCoef (std::max (5.0f, set.releaseMs) * 0.25f, fs);
    vibPhase += (float) (5.5 / fs);
    if (vibPhase >= 1.0f) vibPhase -= 1.0f;
    const float vib = (float) std::sin (kTwoPi * vibPhase) * clampf (set.vibrato + wheel, 0.0f, 1.0f) * 0.6f;
    const int uni = std::max (1, std::min (kMaxUnison, set.unison));
    const float uniNorm = 1.0f / std::sqrt ((float) uni);

    float out = 0;
    for (auto& v : voices)
    {
        if (! v.active) continue;
        v.current = v.target + glideC * (v.current - v.target);
        if (v.releasing)
        {
            v.env *= relC;
            if (v.env < 1.0e-4f) { v.active = false; continue; }
        }
        else
            v.env = 1.0f + attC * (v.env - 1.0f);

        const float pitch = v.current + bend + vib;
        const float f = 440.0f * std::pow (2.0f, (pitch - 69.0f) / 12.0f);
        const float dt0 = (float) (f / fs);
        float s = 0;
        for (int u = 0; u < uni; ++u)
        {
            const float off = uni == 1 ? 0.0f : ((float) u / (float) (uni - 1) * 2.0f - 1.0f) * set.detuneCents / 100.0f;
            s += osc (set.wave, v.phase[u], dt0 * std::pow (2.0f, off / 12.0f), set.pulseWidth);
        }
        s *= uniNorm;
        if (set.osc2Mix > 0.001f)
        {
            const float dt2 = dt0 * std::pow (2.0f, (float) set.osc2Semi / 12.0f + set.detuneCents * 0.003f / 12.0f);
            s = s * (1.0f - 0.5f * set.osc2Mix) + osc (set.wave, v.phase2, std::min (dt2, 0.45f), set.pulseWidth) * set.osc2Mix * 0.8f;
        }
        if (set.sub > 0.001f) s += osc (WSquare, v.subPhase, dt0 * 0.5f, 0.5f) * set.sub * 0.6f;
        if (set.noise > 0.001f) s += rng.white() * set.noise * 0.7f;
        out += s * v.env * (0.35f + 0.65f * v.vel);
    }
    return out * 0.35f;
}

// ============================================================================ PitchTracker
namespace { constexpr int kYinWindow = 384, kYinHop = 128; }

void PitchTracker::prepare (double sampleRate)
{
    fs = sampleRate;
    decim = std::max (1, (int) std::lround (fs / 12000.0));
    fsd = fs / decim;
    lp1.setLowpass (fs, std::min (1500.0, fsd * 0.4), 0.54);
    lp2.setLowpass (fs, std::min (1500.0, fsd * 0.4), 1.31);
    const int tauMax = (int) (fsd / 60.0);
    buf.assign ((size_t) (kYinWindow + tauMax + 8), 0.0f);
    frame.assign (buf.size(), 0.0f);
    diff.assign ((size_t) tauMax + 2, 0.0f);
    w = 0; decimCount = 0; hopCount = 0; acc = 0; freq = 0; conf = 0; level = 0;
}

void PitchTracker::push (float x)
{
    const float y = lp2.process (lp1.process (x));
    acc += y;
    if (++decimCount < decim) return;
    const float s = acc / (float) decim;
    acc = 0; decimCount = 0;
    buf[(size_t) w] = s;
    if (++w >= (int) buf.size()) w = 0;
    level = std::max (std::fabs (s), level * 0.999f);
    if (++hopCount >= kYinHop) { hopCount = 0; analyse(); }
}

void PitchTracker::analyse()
{
    const int len = (int) buf.size();
    for (int i = 0; i < len; ++i) frame[(size_t) i] = buf[(size_t) ((w + i) % len)];
    const int tauMax = (int) diff.size() - 2, tauMin = std::max (2, (int) (fsd / 1000.0));

    double energy = 0;
    for (int j = 0; j < kYinWindow; ++j) energy += frame[(size_t) j] * frame[(size_t) j];
    if (energy / kYinWindow < 1.0e-6) { freq = 0; conf = 0; return; }   // below ~-60 dBFS: treat as silence

    diff[0] = 1;
    double running = 0;
    int best = -1;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        double d = 0;
        for (int j = 0; j < kYinWindow; ++j)
        {
            const float e = frame[(size_t) j] - frame[(size_t) (j + tau)];
            d += e * e;
        }
        running += d;
        diff[(size_t) tau] = (float) (running > 0 ? d * tau / running : 1.0);
    }
    for (int tau = tauMin; tau < tauMax; ++tau)
        if (diff[(size_t) tau] < 0.15f)
        {
            while (tau + 1 < tauMax && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
            best = tau;
            break;
        }
    if (best < 0) { freq = 0; conf = 0; return; }

    const float a = diff[(size_t) best - 1], b = diff[(size_t) best], c = diff[(size_t) best + 1];
    const float den = a - 2 * b + c;
    const float shift = std::fabs (den) > 1.0e-9f ? 0.5f * (a - c) / den : 0.0f;
    freq = (float) (fsd / (best + clampf (shift, -1.0f, 1.0f)));
    conf = 1.0f - b;
}
} // namespace mp
