// Parameter table shared by the engine, the presets and the plugin wrapper.
// Every parameter is a float in a Params array; choice/int/bool parameters hold whole numbers.
#pragma once

#include <array>
#include <cstring>
#include <string>

namespace mp
{
enum class Kind { Float, Int, Choice, Bool };

struct ParamDef
{
    const char* id;
    const char* name;
    Kind kind;
    float min, max, def;
    float skewCentre;      // for Float: value at the knob's mid point (0 = linear)
    const char* choices;   // '|' separated labels for Choice
    const char* unit;
};

enum P : int
{
    // vocoder core
    kType, kBands, kLowHz, kHighHz, kFormant, kBandwidth, kAttack, kRelease, kEmphasis, kSibilance, kUnvoiced, kRobot, kFreeze,
    // modulator input
    kModSource, kModGain, kGate,
    // carrier
    kCarrier, kKey, kScale, kChord, kOctave, kWave, kPulseWidth, kOsc2Mix, kOsc2Semi, kUnison, kDetune, kNoise, kSub, kGlide,
    kAmpAttack, kAmpRelease, kTone, kVibrato,
    // output
    kSpread, kEnsemble, kSpace, kDrive, kMix, kOutput,
    kNumParams
};

enum Type { TypeClassic, TypeVintage, TypeSpectral, TypeTalkbox, kNumTypes };
enum Carrier { CarSynth, CarAutoPitch, CarDrone, CarNoise, CarExternal };
enum ModSource { ModMain, ModSidechain };
enum Wave { WSaw, WSquare, WPulse, WTriangle, WSine, WBuzz };

inline const ParamDef* paramDefs()
{
    static const ParamDef defs[kNumParams] = {
        { "type",      "Vocoder Type",   Kind::Choice, 0, 3, 0, 0, "Classic|Vintage|Spectral|Talkbox", "" },
        { "bands",     "Bands",          Kind::Int,    4, 40, 20, 0, nullptr, "" },
        { "lowHz",     "Low Band",       Kind::Float,  50, 500, 100, 150, nullptr, "Hz" },
        { "highHz",    "High Band",      Kind::Float,  2000, 16000, 8000, 6000, nullptr, "Hz" },
        { "formant",   "Formant Shift",  Kind::Float,  -12, 12, 0, 0, nullptr, "st" },
        { "bandwidth", "Bandwidth",      Kind::Float,  0, 1, 0.5f, 0, nullptr, "" },
        { "attack",    "Attack",         Kind::Float,  0.5f, 100, 4, 10, nullptr, "ms" },
        { "release",   "Release",        Kind::Float,  5, 1000, 60, 120, nullptr, "ms" },
        { "emphasis",  "Emphasis",       Kind::Float,  0, 1, 0.4f, 0, nullptr, "" },
        { "sibilance", "Sibilance",      Kind::Float,  0, 1, 0.3f, 0, nullptr, "" },
        { "unvoiced",  "Unvoiced Noise", Kind::Float,  0, 1, 0.25f, 0, nullptr, "" },
        { "robot",     "Robot Steps",    Kind::Float,  0, 1, 0, 0, nullptr, "" },
        { "freeze",    "Freeze",         Kind::Bool,   0, 1, 0, 0, nullptr, "" },

        { "modSource", "Modulator Input", Kind::Choice, 0, 1, 0, 0, "Main In|Sidechain", "" },
        { "modGain",   "Input Gain",     Kind::Float,  -24, 24, 0, 0, nullptr, "dB" },
        { "gate",      "Gate",           Kind::Float,  -90, -20, -70, 0, nullptr, "dB" },

        { "carrier",   "Carrier",        Kind::Choice, 0, 4, 0, 0, "Synth (MIDI)|Auto Pitch|Drone|Noise|External", "" },
        { "key",       "Key",            Kind::Choice, 0, 11, 0, 0, "C|C#|D|D#|E|F|F#|G|G#|A|A#|B", "" },
        { "scale",     "Scale",          Kind::Choice, 0, 8, 0, 0, "Chromatic|Major|Minor|Dorian|Mixolydian|Pentatonic|Minor Pent|Harmonic Minor|Blues", "" },
        { "chord",     "Chord",          Kind::Choice, 0, 15, 0, 0,
          "Single|Octaves|Fifth|Power|Major|Minor|Sus2|Sus4|Maj7|Min7|Dom7|Add9|Fourths|Cluster|Scale Triad|Scale 7th", "" },
        { "octave",    "Octave",         Kind::Int,    -3, 3, 0, 0, nullptr, "" },
        { "wave",      "Wave",           Kind::Choice, 0, 5, 0, 0, "Saw|Square|Pulse|Triangle|Sine|Buzz", "" },
        { "pulseWidth","Pulse Width",    Kind::Float,  0.05f, 0.5f, 0.25f, 0, nullptr, "" },
        { "osc2Mix",   "Osc 2 Mix",      Kind::Float,  0, 1, 0, 0, nullptr, "" },
        { "osc2Semi",  "Osc 2 Interval", Kind::Int,    -24, 24, 12, 0, nullptr, "st" },
        { "unison",    "Unison",         Kind::Int,    1, 7, 1, 0, nullptr, "" },
        { "detune",    "Detune",         Kind::Float,  0, 50, 10, 0, nullptr, "ct" },
        { "noise",     "Carrier Noise",  Kind::Float,  0, 1, 0.05f, 0, nullptr, "" },
        { "sub",       "Sub Osc",        Kind::Float,  0, 1, 0, 0, nullptr, "" },
        { "glide",     "Glide",          Kind::Float,  0, 1000, 0, 150, nullptr, "ms" },
        { "ampAttack", "Amp Attack",     Kind::Float,  0, 2000, 5, 200, nullptr, "ms" },
        { "ampRelease","Amp Release",    Kind::Float,  5, 4000, 150, 400, nullptr, "ms" },
        { "tone",      "Carrier Tone",   Kind::Float,  -1, 1, 0, 0, nullptr, "" },
        { "vibrato",   "Vibrato",        Kind::Float,  0, 1, 0, 0, nullptr, "" },

        { "spread",    "Stereo Spread",  Kind::Float,  0, 1, 0.3f, 0, nullptr, "" },
        { "ensemble",  "Ensemble",       Kind::Float,  0, 1, 0, 0, nullptr, "" },
        { "space",     "Space",          Kind::Float,  0, 1, 0, 0, nullptr, "" },
        { "drive",     "Drive",          Kind::Float,  0, 1, 0, 0, nullptr, "" },
        { "mix",       "Mix",            Kind::Float,  0, 1, 1, 0, nullptr, "" },
        { "output",    "Output",         Kind::Float,  -24, 12, 0, 0, nullptr, "dB" },
    };
    return defs;
}

using Params = std::array<float, kNumParams>;

inline Params defaultParams()
{
    Params p {};
    for (int i = 0; i < kNumParams; ++i) p[(size_t) i] = paramDefs()[i].def;
    return p;
}

inline int paramIndex (const char* id)
{
    for (int i = 0; i < kNumParams; ++i)
        if (std::strcmp (paramDefs()[i].id, id) == 0) return i;
    return -1;
}

// Returns the index of a choice label (case- and space-insensitive), or -1.
inline int choiceIndex (int param, const std::string& label)
{
    auto norm = [] (const std::string& s)
    {
        std::string r;
        for (char c : s)
            if (c != ' ' && c != '_' && c != '(' && c != ')') r += (char) ((c >= 'A' && c <= 'Z') ? c + 32 : c);
        return r;
    };
    const char* ch = paramDefs()[param].choices;
    if (ch == nullptr) return -1;
    const std::string want = norm (label);
    int idx = 0;
    std::string cur;
    for (const char* c = ch;; ++c)
    {
        if (*c == '|' || *c == 0)
        {
            if (norm (cur) == want) return idx;
            ++idx; cur.clear();
            if (*c == 0) break;
        }
        else cur += *c;
    }
    return -1;
}

inline std::string choiceLabel (int param, int index)
{
    const char* ch = paramDefs()[param].choices;
    if (ch == nullptr) return {};
    int idx = 0; std::string cur;
    for (const char* c = ch;; ++c)
    {
        if (*c == '|' || *c == 0) { if (idx == index) return cur; ++idx; cur.clear(); if (*c == 0) break; }
        else cur += *c;
    }
    return {};
}
} // namespace mp
