#include "Presets.h"
#include "Dsp.h"
#include <cstdlib>
#include <sstream>

namespace mp
{
// ---------------------------------------------------------------------------- categories
// Every category's base recipe is applied over the defaults; presets then override.
static const PresetCategory kCategories[] = {
    /* 0 */ { "Classic Robots",     "type=Classic bands=20 wave=Saw" },
    /* 1 */ { "Vintage Machines",   "type=Vintage bands=16 drive=0.15 wave=Saw" },
    /* 2 */ { "Choirs & Ensembles", "type=Classic bands=24 unison=5 detune=18 ensemble=0.5 space=0.35 ampAttack=60 ampRelease=500 vibrato=0.15" },
    /* 3 */ { "Talkbox",            "type=Talkbox wave=Saw sibilance=0.15 unvoiced=0.15 glide=40" },
    /* 4 */ { "Spectral",           "type=Spectral bands=32 wave=Saw" },
    /* 5 */ { "Auto Harmony",       "carrier=AutoPitch type=Classic bands=24 glide=30 gate=-55 key=C scale=Major" },
    /* 6 */ { "Drones & Pads",      "carrier=Drone type=Classic bands=24 unison=5 detune=14 ensemble=0.6 space=0.5 ampAttack=400 ampRelease=1500" },
    /* 7 */ { "Whisper & Noise",    "carrier=Noise type=Classic bands=28 sibilance=0.4 unvoiced=0" },
    /* 8 */ { "Leads",              "type=Classic bands=24 glide=60 ampRelease=120 vibrato=0.1" },
    /* 9 */ { "Bass",               "type=Classic bands=16 octave=-1 sub=0.5 lowHz=70 highHz=6000 ampRelease=90" },
    /* 10 */ { "External Carrier",  "carrier=External type=Classic bands=24 modSource=MainIn" },
    /* 11 */ { "FX & Lo-Fi",        "type=Classic bands=12" },
    /* 12 */ { "Formant & Character", "type=Spectral bands=28 wave=Saw" },
};

// ---------------------------------------------------------------------------- presets
static const PresetDef kPresets[] = {
    // ===== Classic Robots
    { 0, "Init Vocoder", "" },
    { 0, "Robot Voice 101", "bands=16 wave=Square attack=2 release=40" },
    { 0, "Circuit Speaker", "bands=12 wave=Pulse pulseWidth=0.18 release=35" },
    { 0, "Tin Announcer", "bands=10 wave=Square highHz=5000 lowHz=200 sibilance=0.1" },
    { 0, "Clear Diction", "bands=32 wave=Buzz sibilance=0.5 unvoiced=0.4 emphasis=0.6" },
    { 0, "Metal Choir Boy", "bands=24 wave=Saw octave=1 unison=3 detune=8" },
    { 0, "Servo Speech", "bands=14 wave=Square attack=1 release=25 robot=0.25" },
    { 0, "Mainframe Monologue", "bands=18 wave=Buzz formant=-2 octave=-1" },
    { 0, "Unit Seven", "bands=8 wave=Pulse pulseWidth=0.1 release=20 robot=0.4" },
    { 0, "Polished Chrome", "bands=28 wave=Saw unison=2 detune=6 ensemble=0.3" },
    { 0, "Factory Floor", "bands=16 wave=Square drive=0.35 space=0.25" },
    { 0, "Brass Robot", "bands=20 wave=Saw osc2Mix=0.4 osc2Semi=7 bandwidth=0.35" },
    { 0, "Telephone Bot", "bands=10 lowHz=350 highHz=3400 wave=Square" },
    { 0, "Satellite Relay", "bands=14 lowHz=250 highHz=4500 wave=Pulse pulseWidth=0.3 space=0.3 drive=0.2" },
    { 0, "Bandpass Butler", "bands=22 bandwidth=0.25 wave=Saw" },
    { 0, "Soft Synthetic", "bands=24 wave=Triangle bandwidth=0.7 release=90 tone=0.4" },
    { 0, "Hard Edges", "bands=12 attack=0.5 release=12 wave=Square bandwidth=0.2" },
    { 0, "Smooth Operator", "bands=30 attack=8 release=140 wave=Saw tone=-0.2 ensemble=0.25" },
    { 0, "Forty Band Master", "bands=40 wave=Buzz bandwidth=0.45 sibilance=0.4" },
    { 0, "Eight Band Basic", "bands=8 wave=Saw" },
    { 0, "Octave Robot", "bands=20 wave=Saw osc2Mix=0.6 osc2Semi=-12" },
    { 0, "Fifth Gear", "bands=20 wave=Saw chord=Fifth" },
    { 0, "Wide Machine", "bands=24 wave=Saw spread=0.9 ensemble=0.3" },
    { 0, "Pocket Translator", "bands=12 lowHz=300 highHz=5000 wave=Pulse pulseWidth=0.22 octave=1" },

    // ===== Vintage Machines
    { 1, "Seventies Studio", "" },
    { 1, "Ten Band Strings", "bands=10 wave=Saw unison=3 detune=12 ensemble=0.6" },
    { 1, "Analog Warmth", "bands=14 tone=-0.3 drive=0.3 release=90" },
    { 1, "Rack Unit Gray", "bands=16 wave=Pulse pulseWidth=0.3" },
    { 1, "Disco Vox", "bands=16 wave=Saw unison=2 detune=10 ensemble=0.45 space=0.2" },
    { 1, "Krautbahn", "bands=12 wave=Square release=45 drive=0.25" },
    { 1, "Tape Era", "bands=12 tone=-0.4 drive=0.4 highHz=6000 ensemble=0.2" },
    { 1, "Late Night Radio", "bands=10 lowHz=200 highHz=4500 drive=0.3 space=0.25" },
    { 1, "Console Glow", "bands=18 wave=Saw drive=0.2 ensemble=0.3" },
    { 1, "Mellow Filterbank", "bands=14 bandwidth=0.8 release=120 tone=-0.2" },
    { 1, "Gritty Transistor", "bands=12 drive=0.7 wave=Square" },
    { 1, "Valve Vocoder", "bands=16 drive=0.5 tone=-0.15 release=80" },
    { 1, "Funk Machine", "bands=14 wave=Pulse pulseWidth=0.2 attack=1 release=30 glide=30" },
    { 1, "Twenty Channel Classic", "bands=20 wave=Saw bandwidth=0.45" },
    { 1, "Studio B Choir", "bands=16 unison=4 detune=14 ensemble=0.6 space=0.3 ampAttack=40 ampRelease=400" },
    { 1, "Electro Breaker", "bands=10 wave=Square release=25 sibilance=0.2" },
    { 1, "Rolling Wheels", "bands=12 wave=Saw osc2Mix=0.5 osc2Semi=12 drive=0.3" },
    { 1, "Old Television", "bands=8 lowHz=300 highHz=3500 drive=0.45" },
    { 1, "Smoky Lounge", "bands=16 tone=-0.35 ensemble=0.35 space=0.45 release=150" },
    { 1, "Dusty Patchbay", "bands=12 drive=0.35 robot=0.15" },
    { 1, "Brushed Aluminium", "bands=18 wave=Pulse pulseWidth=0.35 ensemble=0.25" },
    { 1, "Sixteen Channel Sweep", "bands=16 bandwidth=0.3 attack=2 release=60" },
    { 1, "Warm Ten", "bands=10 tone=-0.25 bandwidth=0.7" },
    { 1, "Vintage Unison", "bands=14 unison=3 detune=9" },

    // ===== Choirs & Ensembles
    { 2, "Cathedral Choir", "space=0.65 chord=Octaves" },
    { 2, "Angelic Pad", "octave=1 wave=Saw tone=0.3 space=0.6" },
    { 2, "Mixed Voices", "chord=Major unison=4" },
    { 2, "Minor Chapel", "chord=Minor space=0.55" },
    { 2, "Gospel Stack", "chord=Dom7 unison=3 detune=12" },
    { 2, "Monastery", "octave=-1 wave=Buzz chord=Octaves space=0.7 tone=-0.2" },
    { 2, "Glass Ensemble", "wave=Triangle unison=6 detune=10 tone=0.4" },
    { 2, "String Voices", "wave=Saw unison=7 detune=22 ensemble=0.8" },
    { 2, "Velvet Section", "wave=Saw unison=5 detune=16 tone=-0.3 ensemble=0.7" },
    { 2, "Children's Choir", "octave=1 formant=3 unison=4 detune=12" },
    { 2, "Bass Chorus", "octave=-1 formant=-3 unison=4 detune=12" },
    { 2, "Shimmer Hall", "octave=1 osc2Mix=0.4 osc2Semi=12 space=0.85" },
    { 2, "Hymnal", "chord=Sus4 unison=3 space=0.5" },
    { 2, "Ninth Heaven", "chord=Add9 space=0.6" },
    { 2, "Seventh Choir", "chord=Maj7 unison=4 detune=10" },
    { 2, "Ensemble Haze", "unison=7 detune=35 ensemble=1 space=0.5" },
    { 2, "Chamber Group", "unison=3 detune=8 space=0.3 bands=28" },
    { 2, "Spectral Choir", "type=Spectral bands=36 unison=5 space=0.55" },
    { 2, "Breathy Chorus", "noise=0.25 unison=4 sibilance=0.5 unvoiced=0.4" },
    { 2, "Stadium Crowd", "unison=7 detune=45 space=0.8 spread=1" },
    { 2, "Fourths Chant", "chord=Fourths octave=-1 space=0.5" },
    { 2, "Cluster Voices", "chord=Cluster unison=2 space=0.6" },
    { 2, "Slow Swell Choir", "ampAttack=900 ampRelease=2500 space=0.7" },
    { 2, "Vintage Choir Bank", "type=Vintage bands=16 drive=0.2" },

    // ===== Talkbox
    { 3, "Talkbox Classic", "" },
    { 3, "Funky Tube", "wave=Square glide=60 octave=0" },
    { 3, "West Coast Lead", "wave=Saw glide=90 vibrato=0.2 ensemble=0.2" },
    { 3, "Mouth Organ", "wave=Buzz emphasis=0.6" },
    { 3, "Guitar Talk", "wave=Pulse pulseWidth=0.15 drive=0.4" },
    { 3, "Talkbox Bass", "octave=-1 wave=Square sub=0.5" },
    { 3, "Bright Talker", "emphasis=0.9 wave=Saw tone=0.3" },
    { 3, "Dark Talker", "emphasis=0.1 tone=-0.4" },
    { 3, "Talk Chords", "chord=Min7 unison=2 detune=6" },
    { 3, "Roller Skate Jam", "wave=Saw glide=120 drive=0.2 ensemble=0.3" },
    { 3, "Plastic Hose", "bandwidth=0.8 wave=Square" },
    { 3, "Tight Formants", "bandwidth=0.05 wave=Saw" },
    { 3, "Talkbox Up", "formant=4 wave=Saw" },
    { 3, "Talkbox Down", "formant=-4 wave=Saw" },
    { 3, "Gritty Box", "drive=0.6 wave=Square" },
    { 3, "Talkbox Pad", "unison=5 detune=14 ampAttack=200 ampRelease=900 space=0.4" },
    { 3, "Wah Talk", "wave=Pulse pulseWidth=0.3 bandwidth=0.2 emphasis=0.7" },
    { 3, "Retro Box", "type=Talkbox drive=0.25 tone=-0.2 space=0.2" },
    { 3, "Robot Talk", "robot=0.3 wave=Square" },
    { 3, "Synth Talk Duo", "osc2Mix=0.5 osc2Semi=7 wave=Saw" },
    { 3, "Auto Talk", "carrier=AutoPitch gate=-55 scale=Minor" },
    { 3, "Talk Drone", "carrier=Drone chord=Fifth ampAttack=300 ampRelease=1000" },
    { 3, "Breathy Box", "noise=0.3 unvoiced=0.4" },
    { 3, "Talkbox Wide", "ensemble=0.6 spread=1" },

    // ===== Spectral
    { 4, "Spectral Clean", "" },
    { 4, "Hi-Res Voice", "bands=40 bandwidth=0.2 sibilance=0.3" },
    { 4, "Smooth Spectrum", "bands=16 bandwidth=0.8" },
    { 4, "Crystal", "wave=Triangle octave=1 bands=36 tone=0.5" },
    { 4, "Spectral Strings", "unison=6 detune=18 ensemble=0.6" },
    { 4, "Vowel Glass", "wave=Sine osc2Mix=0.5 osc2Semi=19 bands=40" },
    { 4, "FFT Blur", "bands=8 bandwidth=1 release=300" },
    { 4, "Spectral Robot", "robot=0.35 bands=24" },
    { 4, "Wide Spectrum", "spread=1 ensemble=0.3" },
    { 4, "Spectral Choir Hall", "unison=5 detune=14 space=0.6 ampAttack=80" },
    { 4, "Precise Lead", "bands=40 glide=40 vibrato=0.15" },
    { 4, "Lo-Res Spectral", "bands=6 bandwidth=0.6" },
    { 4, "Spectral Smear", "attack=40 release=600" },
    { 4, "Buzz Spectrum", "wave=Buzz bands=36" },
    { 4, "Square Bins", "wave=Square bands=24" },
    { 4, "Airy Top", "highHz=16000 sibilance=0.5 tone=0.4" },
    { 4, "Low Mid Focus", "lowHz=80 highHz=3500" },
    { 4, "Spectral Freeze Pad", "release=1000 attack=60 ampAttack=300 ampRelease=1500 space=0.6" },
    { 4, "Pulse Spectrum", "wave=Pulse pulseWidth=0.12" },
    { 4, "Octave Spectrum", "osc2Mix=0.5 osc2Semi=-12" },
    { 4, "Spectral Detail", "bands=40 attack=1 release=30 emphasis=0.7" },
    { 4, "Vapor Voice", "bands=12 space=0.7 ensemble=0.5 tone=-0.3" },
    { 4, "Chord Spectrum", "chord=Maj7" },
    { 4, "Spectral Noise Mix", "noise=0.3 unvoiced=0.3" },

    // ===== Auto Harmony (no MIDI needed)
    { 5, "Auto Robot", "" },
    { 5, "Auto Major Triads", "chord=ScaleTriad" },
    { 5, "Auto Minor Triads", "chord=ScaleTriad scale=Minor key=A" },
    { 5, "Auto Sevenths", "chord=Scale7th unison=2 detune=6" },
    { 5, "Pitch Follower", "chord=Single glide=10" },
    { 5, "Auto Octaves", "chord=Octaves" },
    { 5, "Auto Power", "chord=Power wave=Square drive=0.3" },
    { 5, "Auto Choir", "chord=ScaleTriad unison=5 detune=16 ensemble=0.6 space=0.45 ampAttack=60" },
    { 5, "Auto Bass Robot", "octave=-1 chord=Single sub=0.5 wave=Square" },
    { 5, "Auto Chipmunk", "octave=1 formant=5 chord=Single" },
    { 5, "Auto Giant", "octave=-1 formant=-5 chord=Single" },
    { 5, "Pentatonic Robot", "scale=Pentatonic chord=Single" },
    { 5, "Blues Machine", "scale=Blues chord=Single wave=Square" },
    { 5, "Dorian Stack", "scale=Dorian chord=ScaleTriad key=D" },
    { 5, "Mixolydian Funk", "scale=Mixolydian key=G chord=Single wave=Pulse pulseWidth=0.2" },
    { 5, "Hard Snap", "glide=0 chord=Single wave=Square" },
    { 5, "Slow Glide Pitch", "glide=250 chord=Single" },
    { 5, "Auto Talkbox", "type=Talkbox chord=Single" },
    { 5, "Auto Spectral Harmony", "type=Spectral bands=32 chord=ScaleTriad" },
    { 5, "Harmonic Minor Chant", "scale=HarmonicMinor key=E chord=ScaleTriad space=0.4" },
    { 5, "Auto Fifths", "chord=Fifth" },
    { 5, "Chromatic Robot", "scale=Chromatic chord=Single" },
    { 5, "Auto Sus Pad", "chord=Sus2 ampAttack=150 ampRelease=700 space=0.5" },
    { 5, "Minor Pent Lead", "scale=MinorPent key=A chord=Single glide=50 vibrato=0.2" },

    // ===== Drones & Pads (no MIDI needed)
    { 6, "C Major Drone", "chord=Major key=C" },
    { 6, "A Minor Drone", "chord=Minor key=A" },
    { 6, "D Sus Drone", "chord=Sus2 key=D" },
    { 6, "E Fifths", "chord=Fifth key=E octave=-1" },
    { 6, "F Major Seventh", "chord=Maj7 key=F" },
    { 6, "G Dominant Haze", "chord=Dom7 key=G" },
    { 6, "Bb Ninth Bloom", "chord=Add9 key=A#" },
    { 6, "F# Minor Seventh", "chord=Min7 key=F#" },
    { 6, "Low Octave Drone", "chord=Octaves key=C octave=-1 wave=Buzz" },
    { 6, "Fourths Cloud", "chord=Fourths key=D" },
    { 6, "Cluster Fog", "chord=Cluster key=E unison=6 space=0.8" },
    { 6, "Organ Drone", "chord=Major wave=Square unison=1 ensemble=0.3 osc2Mix=0.5 osc2Semi=12" },
    { 6, "Dark Pad", "chord=Minor key=C octave=-1 tone=-0.5" },
    { 6, "Bright Pad", "chord=Major key=G octave=1 tone=0.5" },
    { 6, "Spectral Drone", "type=Spectral bands=32 chord=Maj7" },
    { 6, "Vintage Drone", "type=Vintage bands=14 chord=Minor drive=0.25" },
    { 6, "Talking Drone", "type=Talkbox chord=Sus4 key=D" },
    { 6, "Power Drone", "chord=Power key=E wave=Square drive=0.4" },
    { 6, "Single Note Hum", "chord=Single key=A wave=Saw unison=3" },
    { 6, "Frozen Choir", "chord=Major freeze=0 release=600 space=0.8" },
    { 6, "Glass Harmonics", "chord=Octaves wave=Sine osc2Mix=0.6 osc2Semi=19 octave=1" },
    { 6, "Scale Seventh Pad", "chord=Scale7th scale=Dorian key=D" },
    { 6, "Pulse Pad", "chord=Min7 wave=Pulse pulseWidth=0.2" },
    { 6, "Tidal Pad", "chord=Sus4 key=B release=400 attack=30 space=0.7" },

    // ===== Whisper & Noise
    { 7, "Whisper", "" },
    { 7, "Ghost Voice", "space=0.7 ensemble=0.4 tone=-0.2" },
    { 7, "Radio Static", "lowHz=300 highHz=4000 drive=0.4 bands=16" },
    { 7, "Wind Speaker", "bands=12 release=200 tone=-0.4 space=0.5" },
    { 7, "Breath Pad", "release=400 attack=20 space=0.6" },
    { 7, "Noise Spectral", "type=Spectral bands=36" },
    { 7, "Noise Talkbox", "type=Talkbox" },
    { 7, "Sand Voice", "bands=8 tone=0.4" },
    { 7, "Steam Valve", "bands=10 tone=0.6 sibilance=0.6 attack=0.5" },
    { 7, "Hiss Choir", "bands=24 ensemble=0.7 space=0.6" },
    { 7, "Secret Message", "bands=32 lowHz=200 highHz=7000 mix=0.85" },
    { 7, "Formant Whisper Up", "formant=5 type=Spectral" },
    { 7, "Formant Whisper Down", "formant=-5 type=Spectral" },
    { 7, "Rain Voice", "bands=40 tone=0.3 robot=0.6" },
    { 7, "Vintage Air", "type=Vintage bands=14" },
    { 7, "Shush", "bands=6 highHz=12000 tone=0.5" },
    { 7, "Whisper Stutter", "robot=0.75 bands=16" },
    { 7, "Thin Air", "lowHz=500 highHz=12000 bands=20" },
    { 7, "Cave Whisper", "space=1 tone=-0.3" },
    { 7, "Noise Freeze", "freeze=0 release=900 space=0.5" },
    { 7, "Crushed Whisper", "drive=0.8 bands=10" },
    { 7, "Tiny Whisper", "formant=8 type=Spectral highHz=14000" },
    { 7, "Wide Whisper", "spread=1 ensemble=0.5" },
    { 7, "Breath Texture", "mix=0.6 bands=24" },

    // ===== Leads
    { 8, "Vocal Lead", "" },
    { 8, "Saw Speaker", "wave=Saw unison=3 detune=10" },
    { 8, "Square Lead Talk", "wave=Square octave=1" },
    { 8, "Pulse Width Voice", "wave=Pulse pulseWidth=0.12 vibrato=0.25" },
    { 8, "Super Saw Vox", "wave=Saw unison=7 detune=28 ensemble=0.4 space=0.3" },
    { 8, "Sync-ish Talker", "osc2Mix=0.7 osc2Semi=19 wave=Saw" },
    { 8, "Portamento Bot", "glide=250 wave=Square" },
    { 8, "Screaming Lead", "drive=0.7 wave=Saw tone=0.4 emphasis=0.8" },
    { 8, "Soft Sine Voice", "wave=Sine osc2Mix=0.3 osc2Semi=12 noise=0.1" },
    { 8, "Triangle Talk", "wave=Triangle tone=0.3" },
    { 8, "Buzz Lead", "wave=Buzz glide=30" },
    { 8, "Fifth Lead", "chord=Fifth wave=Saw" },
    { 8, "Octave Lead", "chord=Octaves wave=Square" },
    { 8, "Spectral Lead", "type=Spectral bands=36" },
    { 8, "Talkbox Lead", "type=Talkbox glide=80 vibrato=0.25" },
    { 8, "Vintage Lead", "type=Vintage bands=16 drive=0.3" },
    { 8, "Retro Game Voice", "wave=Pulse pulseWidth=0.25 bands=10 robot=0.2" },
    { 8, "Wide Lead", "spread=1 ensemble=0.5 unison=3" },
    { 8, "Delayed Vibrato Lead", "vibrato=0.4 glide=40" },
    { 8, "Hollow Lead", "wave=Square osc2Mix=0.4 osc2Semi=-12 tone=-0.2" },
    { 8, "Piercing Lead", "highHz=12000 emphasis=0.9 sibilance=0.5" },
    { 8, "Gentle Lead", "attack=8 release=150 ampAttack=40 tone=-0.3" },
    { 8, "Dirty Lead", "drive=0.5 wave=Square" },
    { 8, "Lead Duo", "chord=Major unison=2" },

    // ===== Bass
    { 9, "Talking Bass", "" },
    { 9, "Sub Speaker", "octave=-2 sub=0.8 wave=Sine osc2Mix=0.4 osc2Semi=12" },
    { 9, "Square Bass Vox", "wave=Square" },
    { 9, "Growl Bass", "wave=Saw drive=0.6 formant=-3" },
    { 9, "Funk Bass Talk", "wave=Pulse pulseWidth=0.2 attack=1 release=30 glide=20" },
    { 9, "Reese Talker", "wave=Saw unison=3 detune=22" },
    { 9, "Wobble Mouth", "robot=0.5 wave=Saw drive=0.3" },
    { 9, "Deep Robot", "formant=-6 octave=-1" },
    { 9, "Bass Talkbox", "type=Talkbox wave=Square" },
    { 9, "Spectral Bass", "type=Spectral bands=24" },
    { 9, "Vintage Bass", "type=Vintage bands=12 drive=0.4" },
    { 9, "Octave Bass", "chord=Octaves" },
    { 9, "Power Bass", "chord=Power drive=0.4" },
    { 9, "Buzz Bass", "wave=Buzz" },
    { 9, "Round Bass Voice", "wave=Triangle sub=0.7 tone=-0.3" },
    { 9, "Acid Mouth", "wave=Saw bandwidth=0.15 attack=0.5 release=20 drive=0.35" },
    { 9, "Slide Bass", "glide=180" },
    { 9, "Narrow Bass", "highHz=3000 bands=10" },
    { 9, "Sub Whisper Bass", "noise=0.3 sub=0.8" },
    { 9, "Ogre", "formant=-9 octave=-2 sub=0.5" },
    { 9, "Bass Choir", "unison=5 detune=14 ensemble=0.5" },
    { 9, "Dub Talker", "space=0.4 drive=0.2 release=150" },
    { 9, "Mono Machine", "unison=1 wave=Pulse pulseWidth=0.35" },
    { 9, "Auto Bass", "carrier=AutoPitch gate=-55 octave=-1 chord=Single" },

    // ===== External Carrier (route a synth, guitar or any track to the sidechain)
    { 10, "Sidechain Classic", "modSource=MainIn" },
    { 10, "Sidechain Vintage", "type=Vintage bands=16" },
    { 10, "Sidechain Spectral", "type=Spectral bands=32" },
    { 10, "Sidechain Talkbox", "type=Talkbox" },
    { 10, "Guitar Talker", "type=Talkbox emphasis=0.6 drive=0.2" },
    { 10, "Pad Speaker", "bands=28 release=150 space=0.3" },
    { 10, "Drum Speech", "bands=16 attack=0.5 release=20" },
    { 10, "Synth Through Voice", "bands=32 sibilance=0.4" },
    { 10, "Reverse Routing", "modSource=Sidechain" },
    { 10, "Reverse Spectral", "modSource=Sidechain type=Spectral bands=32" },
    { 10, "Beat Gate Vocoder", "modSource=Sidechain bands=12 attack=0.5 release=25" },
    { 10, "Wide Sidechain", "spread=1 ensemble=0.4" },
    { 10, "Bright External", "tone=0.6 emphasis=0.7" },
    { 10, "Dark External", "tone=-0.6" },
    { 10, "External Plus Noise", "unvoiced=0.6 sibilance=0.5" },
    { 10, "External Shift Up", "type=Spectral formant=4" },
    { 10, "External Shift Down", "type=Spectral formant=-4" },
    { 10, "External Robot", "robot=0.4" },
    { 10, "External Hi-Res", "bands=40 bandwidth=0.3" },
    { 10, "External Lo-Res", "bands=6" },
    { 10, "External Smear", "type=Spectral attack=40 release=500" },
    { 10, "External Freeze", "type=Spectral freeze=0 release=800 space=0.4" },
    { 10, "External Drive", "drive=0.5" },
    { 10, "External Parallel", "mix=0.6" },

    // ===== FX & Lo-Fi
    { 11, "Robot Stutter", "robot=0.7 wave=Square" },
    { 11, "Bitbox", "robot=0.85 bands=8 wave=Pulse pulseWidth=0.1" },
    { 11, "Frozen Vowel", "release=1000 attack=60 ampRelease=2000 space=0.5" },
    { 11, "Glitch Speaker", "robot=0.5 bands=6 drive=0.5" },
    { 11, "Broken Radio", "lowHz=400 highHz=3000 drive=0.8 robot=0.3" },
    { 11, "Walkie Talkie", "lowHz=450 highHz=3200 bands=8 drive=0.6 wave=Square" },
    { 11, "Megaphone", "lowHz=500 highHz=4000 bands=10 drive=0.9" },
    { 11, "Underwater", "highHz=2000 lowHz=60 bands=10 tone=-0.8 ensemble=0.8" },
    { 11, "Alien Transmission", "formant=7 type=Spectral robot=0.3 space=0.5" },
    { 11, "Monster", "formant=-10 octave=-2 type=Spectral drive=0.3" },
    { 11, "Chipmunk Bot", "formant=10 octave=2 type=Spectral" },
    { 11, "Pitch Smear", "glide=900 wave=Saw" },
    { 11, "Long Tail Space", "space=1 release=500" },
    { 11, "Ring Talker", "wave=Sine osc2Mix=1 osc2Semi=-23" },
    { 11, "Laser Voice", "wave=Pulse pulseWidth=0.05 bands=16 tone=0.7" },
    { 11, "Toy Speaker", "lowHz=500 highHz=5000 bands=6 drive=0.5 octave=1" },
    { 11, "Dial-Up", "robot=0.95 bands=10 wave=Square" },
    { 11, "Ghost Machine", "space=0.9 ensemble=0.9 mix=0.8" },
    { 11, "Haunted Organ", "chord=Cluster wave=Square space=0.7" },
    { 11, "Static Choir", "carrier=Noise robot=0.6 space=0.6" },
    { 11, "Fuzz Throat", "drive=1 wave=Saw" },
    { 11, "Stepped Formants", "type=Spectral robot=0.55" },
    { 11, "Wide Glitch", "robot=0.6 spread=1 ensemble=0.5" },
    { 11, "Freeze Talkbox", "type=Talkbox release=1000 attack=60 ampRelease=1500" },

    // ===== Formant & Character
    { 12, "Character Neutral", "" },
    { 12, "Up Three", "formant=3" },
    { 12, "Up Five", "formant=5" },
    { 12, "Up Seven", "formant=7" },
    { 12, "Down Three", "formant=-3" },
    { 12, "Down Five", "formant=-5" },
    { 12, "Down Seven", "formant=-7" },
    { 12, "Small Creature", "formant=9 octave=1" },
    { 12, "Giant", "formant=-9 octave=-1" },
    { 12, "Bright Persona", "formant=2 tone=0.4 emphasis=0.7" },
    { 12, "Deep Persona", "formant=-2 tone=-0.3" },
    { 12, "Classic Shift Up", "type=Classic bands=24 formant=4" },
    { 12, "Classic Shift Down", "type=Classic bands=24 formant=-4" },
    { 12, "Talkbox Up Five", "type=Talkbox formant=5" },
    { 12, "Talkbox Down Five", "type=Talkbox formant=-5" },
    { 12, "Vintage Shift Up", "type=Vintage bands=16 formant=3" },
    { 12, "Vintage Shift Down", "type=Vintage bands=16 formant=-3" },
    { 12, "Duet Up Down", "formant=4 chord=Octaves" },
    { 12, "Character Choir", "formant=3 unison=5 detune=14 ensemble=0.5 space=0.4" },
    { 12, "Gravel Persona", "formant=-4 drive=0.5 noise=0.2" },
    { 12, "Cartoon Voice", "formant=8 glide=60 vibrato=0.3" },
    { 12, "Robot Giant", "formant=-6 robot=0.3 wave=Square" },
    { 12, "Narrow Character", "lowHz=250 highHz=5000 formant=2" },
    { 12, "Extreme Up", "formant=12 octave=1" },
    { 12, "Extreme Down", "formant=-12 octave=-1" },
};

const PresetCategory* presetCategories (int& count)
{
    count = (int) (sizeof (kCategories) / sizeof (kCategories[0]));
    return kCategories;
}

const PresetDef* presetDefs (int& count)
{
    count = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
    return kPresets;
}

int numPresets() { int n = 0; presetDefs (n); return n; }

bool applyRecipe (Params& p, const char* recipe, std::string* error)
{
    std::istringstream in (recipe != nullptr ? recipe : "");
    std::string tok;
    bool ok = true;
    auto fail = [&] (const std::string& msg) { ok = false; if (error != nullptr) *error += msg + "; "; };
    while (in >> tok)
    {
        const auto eq = tok.find ('=');
        if (eq == std::string::npos) { fail ("missing '=' in " + tok); continue; }
        const std::string key = tok.substr (0, eq), val = tok.substr (eq + 1);
        const int idx = paramIndex (key.c_str());
        if (idx < 0) { fail ("unknown key " + key); continue; }
        const ParamDef& d = paramDefs()[idx];
        float v = 0;
        char* end = nullptr;
        const double num = std::strtod (val.c_str(), &end);
        if (end != nullptr && *end == 0 && ! val.empty()) v = (float) num;
        else if (d.kind == Kind::Choice)
        {
            const int c = choiceIndex (idx, val);
            if (c < 0) { fail ("bad choice " + tok); continue; }
            v = (float) c;
        }
        else { fail ("bad number " + tok); continue; }
        if (v < d.min - 1.0e-6f || v > d.max + 1.0e-6f) { fail ("out of range " + tok); v = clampf (v, d.min, d.max); }
        p[(size_t) idx] = v;
    }
    return ok;
}

Params presetParams (int index)
{
    int n = 0, nc = 0;
    const auto* defs = presetDefs (n);
    const auto* cats = presetCategories (nc);
    Params p = defaultParams();
    if (index < 0 || index >= n) return p;
    applyRecipe (p, cats[defs[index].category].base);
    applyRecipe (p, defs[index].recipe);
    return p;
}
} // namespace mp
