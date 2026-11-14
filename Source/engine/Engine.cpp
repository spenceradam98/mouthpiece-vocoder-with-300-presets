#include "Engine.h"

namespace mp
{
namespace
{
// semitone masks for the Scale parameter (bit n = pitch class n above the key)
constexpr uint16_t kScaleMasks[] = {
    0xFFF,                                                               // Chromatic
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),   // Major
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),   // Minor
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),   // Dorian
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),   // Mixolydian
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 7) | (1 << 9),                          // Pentatonic
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 10),                         // Minor Pent
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 11),   // Harmonic Minor
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 10),              // Blues
};

bool inScale (int note, int key, int scale)
{
    const int pc = ((note - key) % 12 + 12) % 12;
    return (kScaleMasks[std::max (0, std::min (8, scale))] >> pc) & 1;
}

// n-th scale note above `note` (note itself is snapped down into the scale first)
int scaleStep (int note, int key, int scale, int steps)
{
    int n = note;
    while (! inScale (n, key, scale)) --n;
    for (int s = 0; s < steps; ++s)
        do { ++n; } while (! inScale (n, key, scale));
    return n;
}

constexpr int kDroneTag = 1000, kAutoTag = 2000;
}

int Engine::chordIntervals (int chord, int scale, int key, int note, int* out)
{
    static const int fixed[][5] = {
        { 0, -1 },            // Single
        { 0, 12, -1 },        // Octaves
        { 0, 7, -1 },         // Fifth
        { -12, 0, 7, 12, -1 },// Power
        { 0, 4, 7, -1 },      // Major
        { 0, 3, 7, -1 },      // Minor
        { 0, 2, 7, -1 },      // Sus2
        { 0, 5, 7, -1 },      // Sus4
        { 0, 4, 7, 11, -1 },  // Maj7
        { 0, 3, 7, 10, -1 },  // Min7
        { 0, 4, 7, 10, -1 },  // Dom7
        { 0, 4, 7, 14, -1 },  // Add9
        { 0, 5, 10, 15, -1 }, // Fourths
        { 0, 2, 3, 5, -1 },   // Cluster
    };
    if (chord >= 14)   // Scale Triad / Scale 7th: stack diatonic thirds from the scale
    {
        const int sc = scale == 0 ? 1 : scale;   // chromatic has no thirds: use major
        const int root = note;
        int count = 0;
        out[count++] = 0;
        const int base = scaleStep (root, key, sc, 0);
        out[count++] = scaleStep (root, key, sc, 2) - base + (root - base);
        out[count++] = scaleStep (root, key, sc, 4) - base + (root - base);
        if (chord == 15) out[count++] = scaleStep (root, key, sc, 6) - base + (root - base);
        return count;
    }
    const int c = std::max (0, std::min (13, chord));
    int count = 0;
    for (int i = 0; i < 5 && fixed[c][i] != -1; ++i) out[count++] = fixed[c][i];
    return count;
}

float Engine::quantise (float midi, int key, int scale)
{
    const int base = (int) std::lround (midi);
    for (int d = 0; d <= 6; ++d)
    {
        // check the nearer neighbour first
        const int a = midi >= (float) base ? base + d : base - d, b = midi >= (float) base ? base - d : base + d;
        if (inScale (a, key, scale)) return (float) a;
        if (inScale (b, key, scale)) return (float) b;
    }
    return (float) base;
}

void Engine::prepare (double sampleRate, int)
{
    fs = sampleRate;
    synth.prepare (fs);
    tracker.prepare (fs);
    bank.prepare (fs);
    spectral.prepare (fs);
    lpc.prepare (fs);
    ensemble.prepare (fs);
    space.prepare (fs);
    sibHp.setHighpass (fs, 4500.0, 0.7071);
    uvHp.setHighpass (fs, 3000.0, 0.7071);
    emphHp.setHighpass (fs, 1500.0, 0.7071);
    extDc.setHighpass (fs, 20.0, 0.7071);
    toneLp.set (fs, 800.0);
    gateFollow.set (fs, 0.5f, 60.0f);
    uvAll.set (fs, 2.0f, 30.0f);
    uvHigh.set (fs, 2.0f, 30.0f);
    modMeter.set (fs, 1.0f, 300.0f);
    carMeter.set (fs, 1.0f, 300.0f);
    outMeter.set (fs, 1.0f, 300.0f);
    gateAtt = timeCoef (1.0f, fs);
    gateRel = timeCoef (80.0f, fs);
    dryDelay.assign (4096 + 8, 0.0f);
    dryW = 0;
    lastType = -1; lastCarrier = -1; droneSig = -1; autoNote = -1;
    setParams (params);
}

void Engine::reset()
{
    synth.allOff (true);
    bank.reset(); spectral.reset(); lpc.reset();
    std::fill (dryDelay.begin(), dryDelay.end(), 0.0f);
    droneSig = -1; autoNote = -1;
}

int Engine::latencySamples() const { return (int) std::lround (params[kType]) == TypeSpectral ? spectral.latency() : 0; }

void Engine::setParams (const Params& p)
{
    params = p;
    auto i = [&] (int id) { return (int) std::lround (p[(size_t) id]); };
    auto f = [&] (int id) { return p[(size_t) id]; };

    type = std::max (0, std::min (kNumTypes - 1, i (kType)));
    carrier = std::max (0, std::min (4, i (kCarrier)));
    modGain = dbToGain (f (kModGain));
    gateThresh = f (kGate) <= -89.0f ? 0.0f : dbToGain (f (kGate));
    mixWet = f (kMix);
    outGain = dbToGain (f (kOutput));
    sibil = f (kSibilance);
    unvoicedAmt = f (kUnvoiced);
    drive = f (kDrive);
    spread = f (kSpread);
    ensembleAmt = f (kEnsemble);
    spaceAmt = f (kSpace);
    emphasis = f (kEmphasis);
    toneLo = dbToGain (-f (kTone) * 9.0f);
    toneHi = dbToGain (f (kTone) * 9.0f);

    Synth::Settings s;
    s.wave = i (kWave); s.unison = i (kUnison); s.osc2Semi = i (kOsc2Semi);
    s.pulseWidth = f (kPulseWidth); s.osc2Mix = f (kOsc2Mix); s.detuneCents = f (kDetune); s.noise = f (kNoise); s.sub = f (kSub);
    s.glideMs = f (kGlide); s.attackMs = f (kAmpAttack); s.releaseMs = f (kAmpRelease); s.vibrato = f (kVibrato);
    synth.setSettings (s);

    const bool frz = f (kFreeze) > 0.5f;
    if (type == TypeClassic || type == TypeVintage)
        bank.configure (i (kBands), f (kLowHz), f (kHighHz), f (kFormant), f (kBandwidth), f (kAttack), f (kRelease),
                        type == TypeVintage, f (kRobot), frz);
    else if (type == TypeSpectral)
        spectral.configure (i (kBands), f (kLowHz), f (kHighHz), f (kFormant), f (kBandwidth), f (kAttack), f (kRelease), f (kRobot), frz);
    else
        lpc.configure (f (kFormant), f (kBandwidth), f (kEmphasis), f (kRobot), frz);

    if (type != lastType)
    {
        bank.reset(); spectral.reset(); lpc.reset();
        lastType = type;
    }
    if (carrier != lastCarrier)
    {
        synth.allOff (false);
        droneSig = -1; autoNote = -1; autoCandidate = -1;
        if (carrier == CarSynth)
            for (int n = 0; n < 128; ++n)
                if (midiHeld[n]) triggerChord (n, n + 12 * i (kOctave), 0.8f);
        lastCarrier = carrier;
    }
}

void Engine::triggerChord (int tag, int baseNote, float vel)
{
    int iv[8];
    const int cnt = chordIntervals ((int) std::lround (params[kChord]), (int) std::lround (params[kScale]),
                                    (int) std::lround (params[kKey]), baseNote, iv);
    for (int k = 0; k < cnt; ++k)
    {
        const int note = baseNote + iv[k];
        if (note >= 0 && note < 128) synth.noteOn (tag, (float) note, vel);
    }
}

void Engine::handleEvent (const NoteEvent& e)
{
    switch (e.type)
    {
        case NoteEvent::NoteOn:
            if (e.note < 0 || e.note > 127) break;
            midiHeld[e.note] = 1;
            if (carrier == CarSynth) triggerChord (e.note, e.note + 12 * (int) std::lround (params[kOctave]), e.value);
            break;
        case NoteEvent::NoteOff:
            if (e.note < 0 || e.note > 127) break;
            midiHeld[e.note] = 0;
            if (carrier == CarSynth) synth.noteOff (e.note);
            break;
        case NoteEvent::AllOff:
            std::fill (std::begin (midiHeld), std::end (midiHeld), 0);
            if (carrier == CarSynth) synth.allOff (false);
            break;
        case NoteEvent::PitchBend: synth.setBend (e.value * 2.0f); break;
        case NoteEvent::ModWheel: synth.setWheel (e.value); break;
    }
}

void Engine::updateDrone()
{
    const int sig = (int) params[kKey] + 12 * ((int) params[kChord] + 16 * ((int) params[kScale] + 16 * ((int) params[kOctave] + 3)));
    if (sig == droneSig) return;
    synth.noteOff (kDroneTag);
    droneSig = sig;
    triggerChord (kDroneTag, 48 + (int) std::lround (params[kKey]) + 12 * (int) std::lround (params[kOctave]), 0.85f);
}

void Engine::updateAutoPitch()
{
    const float hz = tracker.frequency();
    if (hz <= 0.0f) return;
    const float midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
    const int q = (int) quantise (midi, (int) std::lround (params[kKey]), (int) std::lround (params[kScale]));
    if (q == autoNote) { autoCount = 0; return; }
    if (q == autoCandidate) ++autoCount;
    else { autoCandidate = q; autoCount = 1; }
    if (autoNote < 0 || autoCount >= 2)   // two agreeing reads before changing note: no warble between neighbours
    {
        synth.noteOff (kAutoTag);
        triggerChord (kAutoTag, q + 12 * (int) std::lround (params[kOctave]), 0.9f);
        autoNote = q;
        autoCount = 0;
    }
}

float Engine::renderSample (float mRaw, float ext, float& l, float& r)
{
    float m = mRaw * modGain;
    if (gateThresh > 0.0f)
    {
        const float lvl = gateFollow.process (m);
        const float target = lvl > gateThresh ? 1.0f : 0.0f;
        gateGain = target + (target > gateGain ? gateAtt : gateRel) * (gateGain - target);
        m *= gateGain;
    }
    modMeter.process (m);

    if (carrier == CarAutoPitch)
    {
        tracker.push (m);
        if (++trackCounter >= 64) { trackCounter = 0; updateAutoPitch(); }
    }
    else if (carrier == CarDrone) updateDrone();

    // unvoiced (fricative) detector: share of high-frequency energy in the modulator
    const float all = uvAll.process (m), hf = uvHigh.process (uvHp.process (m));
    const float uvTarget = all > 1.0e-3f ? clampf ((hf / (all + 1.0e-6f) - 0.35f) / 0.35f, 0.0f, 1.0f) : 0.0f;
    uvState = flushDenormal (uvTarget + 0.995f * (uvState - uvTarget));

    float c = 0;
    switch (carrier)
    {
        case CarNoise: c = rng.white() * 0.5f; break;
        case CarExternal: c = extDc.process (ext); break;
        default: c = synth.process(); break;
    }
    const float lo = toneLp.process (c);
    c = lo * toneLo + (c - lo) * toneHi;
    const float carEnv = carMeter.process (c);
    if (unvoicedAmt > 0.001f) c += rng.white() * unvoicedAmt * uvState * 0.6f * std::min (1.0f, carEnv * 5.0f);

    const float me = m + emphasis * 2.0f * emphHp.process (m);
    switch (type)
    {
        case TypeClassic:
        case TypeVintage: bank.process (me, c, spread, l, r); break;
        case TypeSpectral: spectral.process (me, c, spread, l, r); break;
        default: l = r = lpc.process (m, c); break;
    }

    // dry path, delayed to line up with the Spectral core's latency
    const int len = (int) dryDelay.size();
    dryDelay[(size_t) dryW] = m;
    const int lat = type == TypeSpectral ? spectral.latency() : 0;
    int rd = dryW - lat;
    while (rd < 0) rd += len;
    const float md = dryDelay[(size_t) rd];
    if (++dryW >= len) dryW = 0;

    const float s = sibHp.process (md) * sibil * 0.8f;
    l += s; r += s;
    return md;
}

void Engine::process (const float* mod, const float* ext, float* outL, float* outR, int n, const NoteEvent* events, int numEvents)
{
    int ev = 0;
    const float k = 1.0f + drive * 6.0f;
    for (int i = 0; i < n; ++i)
    {
        while (ev < numEvents && events[ev].offset <= i) handleEvent (events[ev++]);
        float l = 0, r = 0;
        const float dry = renderSample (mod != nullptr ? mod[i] : 0.0f, ext != nullptr ? ext[i] : 0.0f, l, r);
        if (drive > 0.001f)
        {
            l = std::tanh (l * k) / k * (1.0f + drive);
            r = std::tanh (r * k) / k * (1.0f + drive);
        }
        ensemble.process (l, r, ensembleAmt);
        space.process (l, r, spaceAmt);
        l = (l * mixWet + dry * (1.0f - mixWet)) * outGain;
        r = (r * mixWet + dry * (1.0f - mixWet)) * outGain;
        if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0; reset(); }   // never pass NaN/inf to the host
        outMeter.process (0.5f * (l + r));
        outL[i] = l; outR[i] = r;
    }
    while (ev < numEvents) handleEvent (events[ev++]);

    // UI read-outs
    float lv[kDisplayBands] {};
    const float lo = params[kLowHz], hi = params[kHighHz];
    if (type == TypeClassic || type == TypeVintage)
    {
        const int nb = bank.numBands();
        for (int b = 0; b < kDisplayBands; ++b)
        {
            const int src = nb > 0 ? std::min (nb - 1, b * nb / kDisplayBands) : 0;
            lv[b] = nb > 0 ? bank.bandLevel (src) * 4.0f : 0.0f;
        }
    }
    else if (type == TypeSpectral) spectral.displayLevels (lv, kDisplayBands, lo, hi);
    else lpc.displayLevels (lv, kDisplayBands, lo, hi);
    for (int b = 0; b < kDisplayBands; ++b) display[(size_t) b].store (lv[b], std::memory_order_relaxed);
    modLevel.store (modMeter.env, std::memory_order_relaxed);
    carLevel.store (carMeter.env, std::memory_order_relaxed);
    outLevel.store (outMeter.env, std::memory_order_relaxed);
    detectedHz.store (carrier == CarAutoPitch ? tracker.frequency() : 0.0f, std::memory_order_relaxed);
    heldNotes.store (synth.activeVoices(), std::memory_order_relaxed);
}
} // namespace mp
