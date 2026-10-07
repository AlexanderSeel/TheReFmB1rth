# TB-303 model and ReBirth-inspired UI direction

The ACID engine is no longer treated as a generic mono subtractive synth. The target is the characteristic behavior of an original TB-303 signal path and sequencer interaction, implemented independently for the FM-1's fixed-point DSP constraints.

## Authentic core

The stock-style path now models:

- one oscillator with **SAW** and an asymmetric **303-SQUARE** derived-style waveform;
- fixed-point PolyBLEP correction on waveform discontinuities;
- **2x internal oscillator/filter processing at 88.2 kHz** before returning to the 44.1 kHz engine rate;
- non-linear **18 dB/oct diode-ladder-style low-pass filtering** informed by Open303's TeeBeeFilter topology;
- an approximately **150 Hz high-pass in the resonance feedback loop**;
- Open303 r5's measured nominal cutoff span of approximately **313.8 Hz to 2394.4 Hz**;
- an exponential fixed-point cutoff-knob law rather than the earlier quadratic approximation;
- Open303 r5's measured cutoff/Env Mod scaler-and-offset relationship, implemented as a dimensionless cutoff multiplier rather than a generic additive-Hz envelope sweep;
- an approximately **15 ms RC-smoothed filter-envelope control path**;
- normal filter-envelope decay controlled over roughly 0.2–2 s, with the default close to the r5 ~1 s behavior;
- accented notes switching the main filter envelope toward the r5 approximately **200 ms** decay behavior;
- the stock-style amplitude contour decaying around the r5 **1230 ms** value;
- normal note release that is effectively very fast and accented note release around **50 ms**;
- legato slide without main-envelope retrigger; the nominal 60 ms Open303 slide setting is represented by the same approximately 12 ms slew time constant used by r5 (`0.2 * slideTime`);
- accented slides changing accent/decay/release behavior without retriggering the main filter envelope;
- phase/filter state reset only after true idle, preserving continuous analog state through a running acid phrase;
- a restrained persistent accent-charge state for repeated-accent behavior;
- output AC coupling/high-pass behavior calibrated near Open303 r5's **44.486 Hz** stage;
- mild bounded non-linearity around the ladder/VCA path.

The implementation is circuit-informed rather than copied wholesale from Open303 or any commercial emulator. Open303's MIT-licensed r5 source is used as a documented technical reference; the embedded implementation is an allocation-free fixed-point adaptation for the JieLi target. Attribution is recorded in `docs/THIRD_PARTY_DSP.md`.

## Measured Env Mod mapping

Open303 r5 derives the envelope mapping from measured TB-303 behavior. The fixed-point port retains the same constants conceptually:

- nominal cutoff endpoints: 313.815 Hz / 2394.412 Hz;
- envelope offset: `0.294391 + 0.048293 * cutoffPosition`;
- low-cutoff envelope scaler: `0.736966 + 3.773996 * envMod`;
- high-cutoff envelope scaler: `0.864345 + 4.194549 * envMod`;
- the effective scaler is interpolated between low/high values according to the exponential cutoff position.

The effective cutoff is therefore based on `nominalCutoff * (offset + scaler * smoothedEnvelope)`, with conservative target bounds applied afterwards. This is intentionally very different from a generic synthesizer's linear `cutoff + envelopeAmount` behavior.

## LFO extension

An original TB-303 has no user LFO. The existing LFO remains an optional TheReFmB1rth extension and defaults to zero amount in the ReBirth-style browser UI.

When enabled, all four selectable shapes are connected to real filter modulation:

- sine-like
- triangle
- saw
- square

Host tests render each shape and require different audio fingerprints, preventing the UI selector from becoming a graph-only control.

## Browser UI

`web/emu/index.html` uses an original ReBirth-inspired composition rather than the previous dashboard:

- two horizontal silver ACID strips;
- SAW / SQUARE switches per ACID machine;
- stock TUNE / Cutoff / Resonance / Env Mod / Decay / Accent controls;
- optional MOD/LFO extension kept visually separate from the stock 303 panel;
- 16 grouped step buttons with accent/slide state;
- A–H pattern bank;
- dark 808/909 rhythm section;
- narrow transport/master strip;
- shared firmware graph model and WASM engine underneath.

The goal is recognizable workflow and density, not a pixel copy of ReBirth RB-338. No original ReBirth artwork, logos or skins are included.

## FM-1 display adaptation

The 240×240 display cannot use the desktop layout literally. `tools/patch_fm1_rebirth_ui.py` adds a compact 16-step lamp strip to ACID graph pages while retaining Felucca's readable card UI. This gives the device the grouped four-by-four visual rhythm of the browser interface without sacrificing legibility or copying original artwork.

Future FM-1 UI work should continue toward dedicated ACID parameter captions and a compact two-page layout:

1. **ACID FILTER** — waveform, tune, cutoff, resonance, env mod, decay;
2. **ACID PERF** — accent, drive, optional MOD controls, pattern/step status.

## Validation

The core has explicit tests for:

- bounded output;
- measured cutoff endpoints/midpoint;
- saw vs 303-square producing different audio;
- stock mode keeping LFO disabled;
- slide preserving the running filter envelope;
- accented slide changing accent state without envelope retrigger;
- normal vs accented VCA release behavior;
- 15 ms envelope RC smoothing beginning below the raw envelope value;
- idle-only analog-path reset;
- accent sweep accumulation;
- all LFO shapes advancing;
- all LFO shapes producing different rendered audio;
- deterministic whole-groovebox audio fingerprint.

The browser and FM-1 target continue to use the same `acid303.c` implementation. A successful render/build is not by itself proof of exact analog equivalence; the final calibration still requires listening/reference comparison and physical FM-1 CPU/underrun validation.
