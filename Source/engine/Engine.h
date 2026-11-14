// Mouthpiece vocoder engine. No JUCE dependency; unit-tested in tests/EngineTests.cpp.
//
// Signal flow (per sample):
//   modulator (mic / main in / sidechain) -> gain -> gate -> [pitch tracker]
//   carrier   (MIDI synth | auto-pitch synth | drone | noise | external) -> tone tilt -> unvoiced noise
//   vocoder core (Classic / Vintage band bank, Spectral STFT, Talkbox LPC) -> sibilance -> drive
//   -> ensemble -> space -> dry/wet -> output
#pragma once

#include "Params.h"
#include "Dsp.h"
#include <atomic>
#include <array>
#include <vector>

namespace mp
{
constexpr int kMaxBands = 40;
constexpr int kDisplayBands = 32;

struct NoteEvent
{
    enum Type { NoteOn, NoteOff, AllOff, PitchBend, ModWheel };
    int offset = 0;
    Type type = NoteOn;
    int note = 0;
    float value = 0;   // velocity 0..1, bend -1..1, wheel 0..1
};

// ---------------------------------------------------------------------------- carrier synth
class Synth
{
public:
    static constexpr int kVoices = 16, kMaxUnison = 7;

    struct Settings
    {
        int wave = WSaw, unison = 1, osc2Semi = 12;
        float pulseWidth = 0.25f, osc2Mix = 0, detuneCents = 10, noise = 0.05f, sub = 0,
              glideMs = 0, attackMs = 5, releaseMs = 150, vibrato = 0;
    };

    void prepare (double fs);
    void setSettings (const Settings& s) { set = s; }
    void noteOn (int tag, float note, float velocity);
    void noteOff (int tag);
    void allOff (bool hard);
    void setBend (float semis) { bend = semis; }
    void setWheel (float w) { wheel = w; }
    float process();
    int activeVoices() const;

private:
    struct Voice
    {
        bool active = false, releasing = false;
        int tag = -1;
        uint32_t age = 0;
        float target = 60, current = 60, vel = 1, env = 0;
        float phase[kMaxUnison] {}, phase2 = 0, subPhase = 0;
    };
    float osc (int wave, float& phase, float dt, float pw);

    double fs = 48000;
    Settings set;
    Voice voices[kVoices];
    uint32_t counter = 0;
    float bend = 0, wheel = 0, vibPhase = 0;
    Rng rng { 12345 };
};

// ---------------------------------------------------------------------------- pitch tracker (YIN on a decimated signal)
class PitchTracker
{
public:
    void prepare (double fs);
    void push (float x);
    float frequency() const { return freq; }   // 0 = unvoiced
    float confidence() const { return conf; }

private:
    void analyse();
    double fs = 48000, fsd = 12000;
    int decim = 4, decimCount = 0, hopCount = 0;
    float acc = 0;
    Biquad lp1, lp2;
    std::vector<float> buf;   // ring of decimated samples
    int w = 0;
    std::vector<float> frame, diff;
    float freq = 0, conf = 0, level = 0;
};

// ---------------------------------------------------------------------------- Classic / Vintage band bank
class BankVocoder
{
public:
    void prepare (double fs);
    void configure (int bands, float lowHz, float highHz, float formantSemis, float bandwidth, float attackMs, float releaseMs,
                    bool vintage, float robot, bool freeze);
    void process (float m, float c, float spread, float& outL, float& outR);
    void reset();
    int numBands() const { return bands; }
    float bandLevel (int i) const { return env[(size_t) i]; }

private:
    double fs = 48000;
    int bands = 0;
    bool vintage = false, freeze = false;
    float cfgLow = -1, cfgHigh = -1, cfgFormant = 99, cfgBw = -1, cfgAtt = -1, cfgRel = -1;
    int cfgBands = -1;
    bool cfgVintage = false;
    std::array<Biquad, kMaxBands> anaA, anaB, synA, synB;
    std::array<float, kMaxBands> env {}, envSmooth {}, held {}, attC {}, relC {}, panL {}, panR {}, trim {}, makeup {};
    float holdPhase = 0, holdInc = 0, gain = 1;
    Rng rng { 777 };
};

// ---------------------------------------------------------------------------- Spectral (STFT envelope transfer)
class SpectralVocoder
{
public:
    void prepare (double fs);
    void configure (int bands, float lowHz, float highHz, float formantSemis, float bandwidth, float attackMs, float releaseMs,
                    float robot, bool freeze);
    void process (float m, float c, float spread, float& outL, float& outR);
    void reset();
    int latency() const { return n; }
    void displayLevels (float* out, int count, float lowHz, float highHz) const;

private:
    void frame (float spread);
    double fs = 48000;
    int n = 1024, hop = 256, pos = 0, hopCount = 0;
    int bands = 20;
    float low = 100, high = 8000, ratio = 1, bandwidth = 0.5f, attC = 0, relC = 0, robot = 0;
    bool freeze = false;
    float holdPhase = 0;
    Fft fft;
    std::vector<float> window, inM, inC, outL, outR, envM, envMs, magM, magC, prefix, envC, tmp;
    std::vector<std::complex<float>> bufM, bufC, specL, specR;
    float lastSpread = 0;
};

// ---------------------------------------------------------------------------- Talkbox (LPC all-pole lattice)
class LpcVocoder
{
public:
    static constexpr int kMaxOrder = 40;
    void prepare (double fs);
    void configure (float formantSemis, float bandwidth, float emphasis, float robot, bool freeze);
    float process (float m, float c);
    void reset();
    void displayLevels (float* out, int count, float lowHz, float highHz) const;

private:
    void analyse();
    double fs = 48000;
    int order = 24, win = 1536, hop = 384, hopCount = 0, w = 0;
    float ratio = 1, bwHz = 60, emphasis = 0.4f, robot = 0;
    bool freeze = false;
    std::vector<float> ring, window, frameBuf;
    std::array<double, kMaxOrder + 1> r {}, a {}, tmpA {};
    std::array<float, kMaxOrder + 1> kOld {}, kNew {}, kCur {}, bState {};
    float gOld = 0, gNew = 0, interp = 1, interpStep = 0, preZ = 0, deZ = 0, cPow = 0.01f, holdPhase = 0;
    std::array<float, kMaxOrder + 1> aDisplay {};
    float gDisplay = 0;
};

// ---------------------------------------------------------------------------- post effects
class Ensemble
{
public:
    void prepare (double fs);
    void process (float& l, float& r, float amount);
private:
    double fs = 48000;
    DelayLine dl, dr;
    float ph[3] { 0, 0.33f, 0.67f };
};

class Space
{
public:
    void prepare (double fs);
    void process (float& l, float& r, float amount);
private:
    static constexpr int kLines = 8;
    DelayLine lines[kLines];
    int len[kLines] {};
    float lp[kLines] {};
    DelayLine preL, preR;
};

// ---------------------------------------------------------------------------- engine
class Engine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void setParams (const Params& p);
    const Params& getParams() const { return params; }

    // mod: modulator (mono). ext: external carrier (mono, may be null). Output stereo.
    void process (const float* mod, const float* ext, float* outL, float* outR, int n, const NoteEvent* events, int numEvents);
    void reset();

    int latencySamples() const;

    // UI read-outs (written on the audio thread, read on the message thread)
    std::array<std::atomic<float>, kDisplayBands> display {};
    std::atomic<float> modLevel { 0 }, carLevel { 0 }, outLevel { 0 }, detectedHz { 0 };
    std::atomic<int> heldNotes { 0 };

    // chord helpers (also used by tests)
    static int chordIntervals (int chord, int scale, int rootPc, int note, int* out);
    static float quantise (float midi, int key, int scale);

private:
    void handleEvent (const NoteEvent& e);
    void updateDrone();
    void updateAutoPitch();
    void triggerChord (int tag, int baseNote, float vel);
    float renderSample (float m, float ext, float& l, float& r);

    double fs = 48000;
    Params params = defaultParams();
    Synth synth;
    PitchTracker tracker;
    BankVocoder bank;
    SpectralVocoder spectral;
    LpcVocoder lpc;
    Ensemble ensemble;
    Space space;
    Rng rng { 4242 };

    int type = TypeClassic, lastType = -1, carrier = CarSynth, lastCarrier = -1;
    float modGain = 1, gateThresh = 0, mixWet = 1, outGain = 1, sibil = 0, unvoicedAmt = 0, drive = 0, spread = 0,
          ensembleAmt = 0, spaceAmt = 0, emphasis = 0, toneLo = 1, toneHi = 1;

    Follower gateFollow, uvAll, uvHigh, modMeter, carMeter, outMeter;
    float gateGain = 0, gateAtt = 0, gateRel = 0, uvState = 0;
    Biquad sibHp, uvHp, emphHp, toneLp1, extDc;
    OnePoleLP toneLp;
    std::vector<float> dryDelay;
    int dryW = 0;

    // drone / auto pitch state
    int droneSig = -1;
    int autoNote = -1, autoCandidate = -1, autoCount = 0, trackCounter = 0;
    int midiHeld[128] {};

    int displayCounter = 0;
};
} // namespace mp
