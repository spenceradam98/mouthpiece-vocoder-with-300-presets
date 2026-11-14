// Engine tests: run with ctest or directly. Exit code 0 = all passed.
#include "Engine.h"
#include "Presets.h"
#include <cstdio>
#include <cmath>
#include <set>
#include <string>
#include <vector>
#include <memory>

using namespace mp;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL %s:%d: ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static constexpr double kFs = 48000.0;

// vowel-like modulator: 120 Hz buzz through one resonance at `formantHz`
static std::vector<float> vowel (double seconds, double formantHz, float level = 0.5f)
{
    std::vector<float> v ((size_t) (seconds * kFs));
    Biquad f; f.setBandpass (kFs, formantHz, 6.0);
    double ph = 0;
    for (auto& s : v)
    {
        ph += 120.0 / kFs; if (ph >= 1) ph -= 1;
        s = f.process ((float) (2 * ph - 1)) * level * 3.0f;
    }
    return v;
}

static std::vector<float> saw (double seconds, double hz, float level = 0.3f)
{
    std::vector<float> v ((size_t) (seconds * kFs));
    for (size_t i = 0; i < v.size(); ++i) v[i] = (float) (2.0 * std::fmod (i * hz / kFs, 1.0) - 1.0) * level;
    return v;
}

struct Out { std::vector<float> l, r; };

static Out render (Engine& e, const std::vector<float>& mod, const std::vector<float>* ext, int note = 48)
{
    Out o; o.l.resize (mod.size()); o.r.resize (mod.size());
    NoteEvent on { 0, NoteEvent::NoteOn, note, 0.8f };
    for (size_t i = 0; i < mod.size(); i += 256)
    {
        const int n = (int) std::min<size_t> (256, mod.size() - i);
        e.process (mod.data() + i, ext != nullptr ? ext->data() + i : nullptr, o.l.data() + i, o.r.data() + i, n,
                   i == 0 && note >= 0 ? &on : nullptr, i == 0 && note >= 0 ? 1 : 0);
    }
    return o;
}

static double rms (const std::vector<float>& v, size_t from = 0)
{
    double s = 0;
    for (size_t i = from; i < v.size(); ++i) s += (double) v[i] * v[i];
    return std::sqrt (s / std::max<size_t> (1, v.size() - from));
}

// energy of v around frequency f (narrow bandpass), skipping the first quarter
static double bandEnergy (const std::vector<float>& v, double f)
{
    Biquad a, b; a.setBandpass (kFs, f, 4.0); b.setBandpass (kFs, f, 4.0);
    double s = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        const float y = b.process (a.process (v[i]));
        if (i > v.size() / 4) s += (double) y * y;
    }
    return s;
}

static void testPresets()
{
    const int n = numPresets();
    CHECK (n >= 300, "expected 300+ presets, got %d", n);
    int nc = 0, np = 0;
    const auto* cats = presetCategories (nc);
    const auto* defs = presetDefs (np);
    std::set<std::string> names;
    std::vector<int> perCat ((size_t) nc, 0);
    for (int i = 0; i < np; ++i)
    {
        CHECK (names.insert (defs[i].name).second, "duplicate preset name %s", defs[i].name);
        CHECK (defs[i].category >= 0 && defs[i].category < nc, "bad category on %s", defs[i].name);
        perCat[(size_t) defs[i].category]++;
        Params p = defaultParams();
        std::string err;
        CHECK (applyRecipe (p, cats[defs[i].category].base, &err), "category %s: %s", cats[defs[i].category].name, err.c_str());
        err.clear();
        CHECK (applyRecipe (p, defs[i].recipe, &err), "preset %s: %s", defs[i].name, err.c_str());
    }
    for (int c = 0; c < nc; ++c) CHECK (perCat[(size_t) c] >= 20, "category %s has only %d presets", cats[c].name, perCat[(size_t) c]);
    std::printf ("presets: %d in %d categories\n", np, nc);
}

// every preset renders finite, bounded, non-silent audio
static void testAllPresetsRender()
{
    const auto mod = vowel (0.6, 900.0);
    const auto ext = saw (0.6, 110.0);
    int silent = 0;
    for (int i = 0; i < numPresets(); ++i)
    {
        Engine e;
        e.setParams (presetParams (i));
        e.prepare (kFs, 256);
        const auto o = render (e, mod, &ext);
        float peak = 0;
        bool finite = true;
        for (size_t k = 0; k < o.l.size(); ++k)
        {
            finite = finite && std::isfinite (o.l[k]) && std::isfinite (o.r[k]);
            peak = std::max ({ peak, std::fabs (o.l[k]), std::fabs (o.r[k]) });
        }
        int nn = 0;
        const char* name = presetDefs (nn)[i].name;
        CHECK (finite, "preset %s produced NaN/inf", name);
        CHECK (peak < 3.0f, "preset %s peaks at %.2f", name, peak);
        const double level = rms (o.l, o.l.size() / 2);
        if (level < 0.003) { ++silent; std::printf ("  quiet preset: %s (rms %.4f)\n", name, level); }
    }
    CHECK (silent == 0, "%d presets were (near) silent", silent);
}

// the vocoder must put the modulator's formant onto the carrier, for every type
static void testFormantTransfer()
{
    const char* typeNames[] = { "Classic", "Vintage", "Spectral", "Talkbox" };
    for (int t = 0; t < kNumTypes; ++t)
    {
        double ratioLow = 0, ratioHigh = 0;
        for (int pass = 0; pass < 2; ++pass)
        {
            Engine e;
            Params p = defaultParams();
            p[kType] = (float) t; p[kSibilance] = 0; p[kUnvoiced] = 0; p[kSpread] = 0; p[kNoise] = 0; p[kBands] = 32;
            e.setParams (p);
            e.prepare (kFs, 256);
            const auto o = render (e, vowel (1.0, pass == 0 ? 700.0 : 2500.0), nullptr);
            const double lowE = bandEnergy (o.l, 700.0), highE = bandEnergy (o.l, 2500.0);
            (pass == 0 ? ratioLow : ratioHigh) = 10.0 * std::log10 ((lowE + 1e-12) / (highE + 1e-12));
        }
        std::printf ("formant %s: 700Hz vowel %.1f dB, 2.5kHz vowel %.1f dB (low/high)\n", typeNames[t], ratioLow, ratioHigh);
        CHECK (ratioLow > 6.0, "%s: 700 Hz formant not transferred (%.1f dB)", typeNames[t], ratioLow);
        CHECK (ratioHigh < (t == TypeVintage ? -4.0 : -6.0), "%s: 2.5 kHz formant not transferred (%.1f dB)", typeNames[t], ratioHigh);
    }
}

// formant shift moves the resonance up
static void testFormantShift()
{
    for (int t : { (int) TypeClassic, (int) TypeSpectral, (int) TypeTalkbox })
    {
        Engine e;
        Params p = defaultParams();
        p[kType] = (float) t; p[kFormant] = 12; p[kSibilance] = 0; p[kUnvoiced] = 0; p[kBands] = 32;
        e.setParams (p);
        e.prepare (kFs, 256);
        const auto o = render (e, vowel (1.0, 700.0), nullptr);
        const double at700 = bandEnergy (o.l, 700.0), at1400 = bandEnergy (o.l, 1400.0);
        CHECK (at1400 > at700, "type %d: +12 st formant shift did not move 700 Hz up (%.3g vs %.3g)", t, at1400, at700);
    }
}

// output level stays in a sane window across types and band counts
static void testLevels()
{
    const auto mod = vowel (1.0, 900.0);
    const double inRms = rms (mod);
    for (int t = 0; t < kNumTypes; ++t)
        for (int bands : { 4, 12, 24, 40 })
        {
            Engine e;
            Params p = defaultParams();
            p[kType] = (float) t; p[kBands] = (float) bands;
            e.setParams (p);
            e.prepare (kFs, 256);
            const auto o = render (e, mod, nullptr);
            const double db = 20.0 * std::log10 (rms (o.l, o.l.size() / 3) / inRms + 1e-12);
            CHECK (db > -12.0 && db < 9.0, "type %d bands %d: level %.1f dB re input", t, bands, db);
        }
}

static void testSilence()
{
    for (int t = 0; t < kNumTypes; ++t)
    {
        Engine e;
        Params p = defaultParams();
        p[kType] = (float) t; p[kNoise] = 0;
        e.setParams (p);
        e.prepare (kFs, 256);
        std::vector<float> mod ((size_t) kFs, 0.0f);
        const auto o = render (e, mod, nullptr);
        CHECK (rms (o.l, o.l.size() / 2) < 1e-4, "type %d: silence in gave rms %.5f", t, rms (o.l, o.l.size() / 2));
    }
}

static void testGate()
{
    Engine e;
    Params p = defaultParams();
    p[kGate] = -30;   // modulator at ~-50 dBFS stays closed
    e.setParams (p);
    e.prepare (kFs, 256);
    const auto o = render (e, vowel (1.0, 900.0, 0.003f), nullptr);
    CHECK (rms (o.l, o.l.size() / 2) < 1e-4, "gate did not close: %.5f", rms (o.l, o.l.size() / 2));
}

static void testPitchTracker()
{
    for (double hz : { 98.0, 220.0, 440.0 })
    {
        PitchTracker t;
        t.prepare (kFs);
        const auto v = saw (0.4, hz, 0.4f);
        for (float s : v) t.push (s);
        const double cents = 1200.0 * std::log2 ((t.frequency() + 1e-9) / hz);
        CHECK (std::fabs (cents) < 20.0, "pitch tracker: %.1f Hz read as %.1f Hz", hz, t.frequency());
    }
}

static void testAutoPitchFollows()
{
    Engine e;
    Params p = defaultParams();
    p[kCarrier] = CarAutoPitch; p[kScale] = 0; p[kChord] = 0; p[kGate] = -90;
    e.setParams (p);
    e.prepare (kFs, 256);
    const auto mod = saw (0.6, 220.0, 0.3f);   // A3 = MIDI 57
    render (e, mod, nullptr, -1);
    CHECK (std::fabs (e.detectedHz.load() - 220.0f) < 3.0f, "auto pitch detected %.1f Hz", e.detectedHz.load());
    CHECK (e.heldNotes.load() >= 1, "auto pitch did not start a carrier voice");
}

static void testChords()
{
    int iv[8];
    int n = Engine::chordIntervals (14, 1, 0, 62, iv);   // D in C major, scale triad -> D F A (minor)
    CHECK (n == 3 && iv[1] == 3 && iv[2] == 7, "scale triad on D in C major: %d %d %d", n, iv[1], iv[2]);
    n = Engine::chordIntervals (14, 1, 0, 60, iv);       // C -> C E G
    CHECK (n == 3 && iv[1] == 4 && iv[2] == 7, "scale triad on C: %d %d", iv[1], iv[2]);
    CHECK (Engine::quantise (61.2f, 0, 1) == 60.0f || Engine::quantise (61.2f, 0, 1) == 62.0f, "quantise C# into C major");
    CHECK (Engine::quantise (64.4f, 0, 1) == 64.0f, "quantise E stays E");
}

static void testDroneNeedsNoMidi()
{
    Engine e;
    Params p = defaultParams();
    p[kCarrier] = CarDrone; p[kChord] = 4;
    e.setParams (p);
    e.prepare (kFs, 256);
    const auto o = render (e, vowel (0.5, 900.0), nullptr, -1);
    CHECK (rms (o.l, o.l.size() / 2) > 0.01, "drone carrier silent without MIDI");
    CHECK (e.heldNotes.load() == 3, "drone major chord should hold 3 voices, holds %d", e.heldNotes.load());
}

static void testLatency()
{
    Engine e;
    Params p = defaultParams();
    p[kType] = TypeSpectral;
    e.setParams (p);
    e.prepare (kFs, 256);
    CHECK (e.latencySamples() == 1024, "spectral latency at 48k: %d", e.latencySamples());
    p[kType] = TypeClassic;
    e.setParams (p);
    CHECK (e.latencySamples() == 0, "classic latency should be 0");
}

static void testOddSampleRates()
{
    for (double fs : { 22050.0, 44100.0, 96000.0, 192000.0 })
        for (int t = 0; t < kNumTypes; ++t)
        {
            Engine e;
            Params p = defaultParams();
            p[kType] = (float) t; p[kHighHz] = 16000; p[kFormant] = 12;
            e.setParams (p);
            e.prepare (fs, 512);
            std::vector<float> mod ((size_t) (fs * 0.3)), l (mod.size()), r (mod.size());
            Rng rng (9);
            for (auto& s : mod) s = rng.white() * 0.3f;
            NoteEvent on { 0, NoteEvent::NoteOn, 60, 1.0f };
            for (size_t i = 0; i < mod.size(); i += 512)
            {
                const int n = (int) std::min<size_t> (512, mod.size() - i);
                e.process (mod.data() + i, nullptr, l.data() + i, r.data() + i, n, i == 0 ? &on : nullptr, i == 0 ? 1 : 0);
            }
            bool ok = true;
            for (float s : l) ok = ok && std::isfinite (s) && std::fabs (s) < 4.0f;
            CHECK (ok, "type %d at %.0f Hz misbehaved", t, fs);
        }
}

int main()
{
    testPresets();
    testChords();
    testPitchTracker();
    testFormantTransfer();
    testFormantShift();
    testLevels();
    testSilence();
    testGate();
    testAutoPitchFollows();
    testDroneNeedsNoMidi();
    testLatency();
    testOddSampleRates();
    testAllPresetsRender();
    if (failures == 0) std::printf ("all engine tests passed\n");
    else std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
