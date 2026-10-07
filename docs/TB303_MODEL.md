# TB-303 model and ReBirth-inspired UI direction

The ACID engine is no longer treated as a generic mono subtractive synth. The target is the characteristic behavior of an original TB-303 signal path and sequencer interaction, implemented independently for the FM-1's fixed-point DSP constraints.

## Authentic core

The stock-style path now models:

- one oscillator with **SAW** and an asymmetric **303-SQUARE** derived-style waveform;
- non-linear **18 dB/oct diode-ladder-style low-pass filtering** rather than the old two-pole filter;
- a high-pass/AC-coupled resonance feedback path;
- a base cutoff control shaped approximately over the hardware's useful 250 Hz–2.4 kHz range before envelope modulation;
- filter-envelope decay over approximately 200 ms–2 s;
- accented notes using the short approximately 200 ms filter-envelope behavior;
- an accent sweep state that does not fully discharge between repeated accents;
- accent interaction with cutoff, resonance and output level;
- a fixed long VCA contour instead of using the generic ADSR as the stock voice shape;
- legato slide without envelope retrigger and a fixed glide near the classic ~60 ms behavior;
- a fixed output coupling/high-pass stage;
- mild bounded non-linearity around the ladder/VCA path.

The implementation is circuit-informed rather than copied from any commercial emulator. It is designed to be deterministic, allocation-free and suitable for the JieLi fixed-point target.

## LFO extension

An original TB-303 has no user LFO. The existing LFO remains an optional TheReFmB1rth extension and defaults to zero amount in the ReBirth-style browser UI.

When enabled, all four selectable shapes are connected to real filter modulation:

- sine-like
- triangle
- saw
- square

Host tests render each shape and require different audio fingerprints, preventing the UI selector from becoming a graph-only control.

## Browser UI

`web/emu/index.html` now uses an original ReBirth-inspired composition rather than the previous dashboard:

- two horizontal silver ACID strips;
- SAW / SQUARE switches per ACID machine;
- Cutoff / Resonance / Env Mod / Decay / Accent / Drive controls;
- optional LFO extension strip;
- 16 grouped step buttons with accent/slide state;
- A–H pattern bank;
- dark 808/909 rhythm section;
- narrow transport/master strip;
- shared firmware graph model and WASM engine underneath.

The goal is recognizable workflow and density, not a pixel copy of ReBirth RB-338. No original ReBirth artwork, logos or skins are included.

## FM-1 display adaptation

The 240×240 display cannot use the desktop layout literally. `tools/patch_fm1_rebirth_ui.py` therefore adds a compact 16-step lamp strip to ACID graph pages while retaining Felucca's readable card UI. This gives the device the grouped four-by-four visual rhythm of the browser interface without sacrificing legibility or copying original artwork.

Future FM-1 UI work should add dedicated ACID parameter captions and a compact two-page layout:

1. **ACID FILTER** — waveform, cutoff, resonance, env mod, decay;
2. **ACID PERF** — accent, drive, optional LFO rate/shape/amount, pattern/step status.

## Validation

The core has explicit tests for:

- bounded output;
- cutoff range;
- saw vs 303-square producing different audio;
- slide preserving the running envelope;
- accent sweep accumulation;
- all LFO shapes advancing;
- all LFO shapes producing different rendered audio;
- deterministic whole-groovebox audio fingerprint.

The browser and FM-1 target continue to use the same `acid303.c` implementation.
