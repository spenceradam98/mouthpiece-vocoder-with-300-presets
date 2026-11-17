# Mouthpiece: features and design notes

## Vocoder engines (Vocoder > Type)

| Type | How it works | Character |
|---|---|---|
| **Classic** | 4 to 40 log-spaced bands, 4th-order band-pass analysis and synthesis filters, per-band envelope followers | Clean, precise, the modern studio vocoder |
| **Vintage** | Same layout with 2nd-order filters, wider bands, a softer two-pole envelope detector, ±1 dB band tolerance and soft saturation | Warmer, smeared, 1970s/80s hardware feel |
| **Spectral** | STFT (1024-point at 44.1/48 kHz, 2048 above), modulator spectral envelope at a resolution set by Bands, carrier spectrum whitened then reshaped | Very intelligible, smooth; adds 1024 samples of reported latency (dry path is compensated) |
| **Talkbox** | Linear prediction (order ~28 at 48 kHz) of the voice every 7.5 ms, carrier played through an all-pole lattice filter with interpolated coefficients | Mouth-tube talkbox sound, strong vowels |

Shared vocoder controls: Bands, Low/High band limits, **Formant** shift (±12 semitones, works in all four
engines), Width (band overlap / talkbox formant bandwidth), Attack, Release, **Sibilance** (adds the voice's
"s" and "t" sounds above 4.5 kHz), **Unvoiced** (injects noise into the carrier when the voice is
unvoiced so consonants stay clear), **Robot** (sample-and-hold of the envelopes, 3 to 43 steps a second),
**Freeze** (holds the current vowel).

## Inputs and routing

- **Modulator input:** main input (mic or vocal track) or the sidechain.
- **Gain** and **Gate** (noise gate for live microphones), **Emphasis** (high-frequency lift on the voice
  before analysis, for clarity).
- **Carrier** sources:
  - **Synth (MIDI)**: 16-voice polyphonic synth played by MIDI or the on-screen keyboard.
  - **Auto Pitch**: tracks the pitch you sing or play (YIN tracker), snaps it to Key + Scale and plays it,
    optionally as a chord. Needs no MIDI.
  - **Drone**: holds Key + Chord. Needs no MIDI.
  - **Noise**: whisper vocoding.
  - **External**: the other input (sidechain, or main input when the voice is on the sidechain), so any
    plugin, synth, guitar or drum track can be the carrier.
  - **Plugin**: a VST3 (or AU on macOS) loaded into Mouthpiece's own carrier slot. Instruments receive the
    MIDI; effects process the external input. Scan / load / replace / open window / remove from the header
    button; the plugin and its state are saved with the session. Built on the hosting code from Scope.
- **Chord** (16 types, including scale-aware triads and sevenths) applies to MIDI notes too: one key plays a
  whole chord. **Octave**, **Key**, **Scale** (9 scales).

## Carrier synth

Saw, Square, Pulse (with PW), Triangle, Sine and Buzz (band-limited impulse train, very bright, ideal for
speech) waves; up to 7 unison voices with Detune; second oscillator with interval and mix; sub oscillator;
noise; glide; amp attack/release; Tone (tilt EQ); Vibrato (mod wheel adds more); pitch bend ±2.

## Output

Stereo **Spread** (alternate bands panned left/right), **Ensemble** (three-voice chorus), **Space** (8-line
reverb), **Drive**, **Mix** (dry voice vs vocoded), **Output** level.

## Presets (313 in 13 categories)

Classic Robots, Vintage Machines, Choirs & Ensembles, Talkbox, Spectral, Auto Harmony, Drones & Pads,
Whisper & Noise, Leads, Bass, External Carrier, FX & Lo-Fi, Formant & Character. Auto Harmony and Drones &
Pads work without any MIDI. Presets are exposed to the host as programs ("Category / Name") and browsed in the
UI with category + preset menus and previous/next buttons. Each preset is a short recipe in
`Source/engine/Presets.cpp`, so adding or editing presets is a one-line change; the tests check every one
for unknown keys, out-of-range values, NaN/inf, clipping and silence.

## UI

1240 x 650, freely resizable (50% to 200%, fixed aspect). Live band display with peak hold, IN / CAR / OUT
meters, a status line that tells you what the current carrier needs (for example "route a synth to the
sidechain"), controls that do not apply to the current mode are dimmed, and an on-screen keyboard.
Themes: **Studio Night** (dark, teal arcs), **Rack '79** (brushed metal, hardware knobs, LED segment
display), **Neon** (synthwave grid and glow), **Paper** (light, flat ink). The theme is saved with the session.

## Compatibility

VST3 and standalone. macOS 11+ universal, Windows (static runtime). Scalar C++ only, no AVX2. CPU on
one core at 48 kHz with worst-case settings (40 bands, 7-voice unison, 4-note chord, ensemble and space on):
about 3 to 5 percent on the build machine.

## User presets

SAVE stores every parameter in `Documents/Mouthpiece/Presets/<name>.mppreset` (plain XML). The **User**
category in the preset menu lists them; previous/next step through them while one is loaded. Files from
future versions with extra parameters still load (missing values fall back to defaults).

## Formats

VST3 + standalone on macOS and Windows, plus AU (`kAudioUnitType_MusicEffect`, so it accepts MIDI) on macOS.

## Tests

`ctest` runs the engine tests (presets, formant transfer for every engine, levels, gate, pitch tracking,
chords, latency, odd sample rates, every preset rendered). With `-DMOUTHPIECE_BUILD_TOOLS=ON` it also runs a
host test that loads a test instrument VST3 into the carrier slot, vocodes it, restores it from saved
session state, and round-trips a user preset.
