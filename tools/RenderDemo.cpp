// Renders a demo reel: one spoken phrase (16-bit mono WAV) through a set of factory presets.
// Usage: mouthpiece_render_demo <voice.wav> <out.wav> [preset names...]
#include "Engine.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using namespace mp;

static bool readWav (const char* path, std::vector<float>& out, int& rate)
{
    FILE* f = std::fopen (path, "rb");
    if (! f) return false;
    std::vector<uint8_t> d;
    uint8_t buf[65536];
    size_t n;
    while ((n = std::fread (buf, 1, sizeof buf, f)) > 0) d.insert (d.end(), buf, buf + n);
    std::fclose (f);
    if (d.size() < 44 || std::memcmp (d.data(), "RIFF", 4) != 0) return false;
    size_t p = 12;
    int channels = 1, bits = 16;
    while (p + 8 <= d.size())
    {
        const uint32_t len = d[p + 4] | (d[p + 5] << 8) | (d[p + 6] << 16) | ((uint32_t) d[p + 7] << 24);
        if (std::memcmp (&d[p], "fmt ", 4) == 0)
        {
            channels = d[p + 10] | (d[p + 11] << 8);
            rate = (int) (d[p + 12] | (d[p + 13] << 8) | (d[p + 14] << 16) | ((uint32_t) d[p + 15] << 24));
            bits = d[p + 22] | (d[p + 23] << 8);
        }
        else if (std::memcmp (&d[p], "data", 4) == 0 && bits == 16)
        {
            const size_t frames = std::min<size_t> (len, d.size() - p - 8) / (2 * (size_t) channels);
            out.resize (frames);
            for (size_t i = 0; i < frames; ++i)
            {
                const uint8_t* s = &d[p + 8 + i * 2 * (size_t) channels];
                out[i] = (float) (int16_t) (s[0] | (s[1] << 8)) / 32768.0f;
            }
            return true;
        }
        p += 8 + len + (len & 1);
    }
    return false;
}

static void writeWav (const char* path, const std::vector<float>& l, const std::vector<float>& r, int rate)
{
    FILE* f = std::fopen (path, "wb");
    if (! f) return;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    const uint32_t bytes = (uint32_t) l.size() * 4;
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) rate); u32 ((uint32_t) rate * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    for (size_t i = 0; i < l.size(); ++i)
    {
        const int16_t a = (int16_t) (clampf (l[i], -1.0f, 1.0f) * 32767.0f), b = (int16_t) (clampf (r[i], -1.0f, 1.0f) * 32767.0f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    if (argc < 3) { std::printf ("usage: %s voice.wav out.wav [preset names...]\n", argv[0]); return 1; }
    std::vector<float> voiceIn;
    int inRate = 0;
    if (! readWav (argv[1], voiceIn, inRate)) { std::printf ("could not read %s (16-bit PCM WAV only)\n", argv[1]); return 1; }

    const int fs = 48000;
    std::vector<float> voice ((size_t) ((double) voiceIn.size() * fs / inRate));
    for (size_t i = 0; i < voice.size(); ++i)   // linear resample is fine for a demo
    {
        const double src = (double) i * inRate / fs;
        const size_t i0 = (size_t) src;
        const float fr = (float) (src - (double) i0);
        const float a = voiceIn[std::min (i0, voiceIn.size() - 1)], b = voiceIn[std::min (i0 + 1, voiceIn.size() - 1)];
        voice[i] = (a + fr * (b - a)) * 1.4f;
    }

    std::vector<std::string> names;
    for (int i = 3; i < argc; ++i) names.push_back (argv[i]);
    if (names.empty())
        names = { "Init Vocoder", "Seventies Studio", "Cathedral Choir", "Talkbox Classic", "Spectral Clean", "Auto Major Triads", "Whisper", "Monster" };

    // chord progression for MIDI-played presets: C, Am, F, G (two-bar-ish changes)
    const int chords[4][4] = { { 48, 55, 60, 64 }, { 45, 52, 57, 60 }, { 41, 53, 57, 60 }, { 43, 50, 55, 59 } };
    const size_t chordLen = voice.size() / 4 + 1;

    std::vector<float> L, R;
    int np = 0;
    const auto* defs = presetDefs (np);
    for (const auto& name : names)
    {
        int idx = -1;
        for (int i = 0; i < np; ++i) if (name == defs[i].name) idx = i;
        if (idx < 0) { std::printf ("no preset named %s\n", name.c_str()); continue; }
        Engine e;
        e.setParams (presetParams (idx));
        e.prepare (fs, 256);
        const size_t total = voice.size() + (size_t) fs;   // one second of tail
        std::vector<float> in (total, 0.0f), l (total), r (total);
        std::copy (voice.begin(), voice.end(), in.begin());
        int current = -1;
        for (size_t i = 0; i < total; i += 256)
        {
            const int n = (int) std::min<size_t> (256, total - i);
            std::vector<NoteEvent> ev;
            const int want = i < voice.size() ? (int) (i / chordLen) : -1;
            if (want != current)
            {
                if (current >= 0) for (int k = 0; k < 4; ++k) ev.push_back ({ 0, NoteEvent::NoteOff, chords[current][k], 0 });
                if (want >= 0) for (int k = 0; k < 4; ++k) ev.push_back ({ 0, NoteEvent::NoteOn, chords[want][k], 0.85f });
                current = want;
            }
            e.process (in.data() + i, nullptr, l.data() + i, r.data() + i, n, ev.data(), (int) ev.size());
        }
        L.insert (L.end(), l.begin(), l.end());
        R.insert (R.end(), r.begin(), r.end());
        std::printf ("%-24s %s\n", name.c_str(), presetCategories (np)[defs[idx].category].name);
        presetDefs (np);
    }
    float peak = 1.0e-6f;
    for (size_t i = 0; i < L.size(); ++i) peak = std::max ({ peak, std::fabs (L[i]), std::fabs (R[i]) });
    const float g = 0.89f / peak;   // normalise the reel to -1 dBFS
    for (size_t i = 0; i < L.size(); ++i) { L[i] *= g; R[i] *= g; }
    writeWav (argv[2], L, R, fs);
    std::printf ("wrote %s (%.1f s)\n", argv[2], (double) L.size() / fs);
    return 0;
}
