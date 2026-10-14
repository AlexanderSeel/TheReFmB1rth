# TB-303 model and ReBirth-inspired UI direction

The ACID engine is no longer treated as a generic mono subtractive synth. The target is the characteristic behavior of an original TB-303 signal path and sequencer interaction, implemented independently for the FM-1's fixed-point DSP constraints.

## Authentic core

The stock-style path now models:

- one oscillator with **SAW** and an asymmetric **303-SQUARE** derived-style waveform;
- compact band-limited mipmapped oscillator tables on the FM-1 target;
- **4x internal oscillator/filter processing at 176.4 kHz** before returning to the 44.1 kHz engine rate;
- non-linear-capable **TB-303/TeeBee four-stage low-pass filtering** informed by Open303's `TeeBeeFilter` topology;
- an approximately **150 Hz high-pass in the resonance feedback loop**;
- Open303 r5's measured nominal cutoff span of approximately **313.8 Hz to 2394.4 Hz**;
- an exponential fixed-point cutoff-knob law rather than a generic linear-Hz control;
- Open303 r5's measured cutoff/Env Mod scaler-and-offset relationship, implemented as a dimensionless cutoff multiplier rather than a generic additive-Hz envelope sweep;
- the main filter-envelope path unsmoothed as in the current Open303 r5 sample loop, while the **accent contribution uses the approximately 15 ms RC path**;
- normal filter-envelope decay controlled over roughly 0.2–2 s, with the default close to the r5 ~1 s behavior;
- accented notes switching the main filter envelope toward the r5 approximately **200 ms** decay behavior;
- the stock-style amplitude contour decaying around the r5 **1230 ms** value;
- normal note release that is effectively very fast and accented note release around **50 ms**;
- legato slide without main-envelope retrigger; the nominal 60 ms Open303 slide setting is represented by the same approximately 12 ms slew time constant used by r5 (`0.2 * slideTime`);
- accented slides changing accent/decay/release behavior without retriggering the main filter envelope;
- phase/filter state reset only after true idle, preserving continuous analog state through a running acid phrase;
- output AC coupling/high-pass behavior calibrated near Open303's **44.486 Hz** pre-ladder and **24.167 Hz** post-ladder stages;
- mild optional bounded per-ACID drive, disabled by default.

The implementation is circuit-informed rather than copied wholesale from Open303 or any commercial emulator. Open303's MIT-licensed source is used as a documented technical reference; the embedded implementation is an allocation-free fixed-point adaptation for the JieLi target. Attribution is recorded in `docs/THIRD_PARTY_DSP.md`.

## Pitch / tuning calibration

The oscillator uses equal temperament with **A4 = 440 Hz** at TUNE center. In addition to the generic A4 arithmetic test, the host suite now checks the service-calibration anchor used for the original TB-303: the low A register resolves to approximately **110 Hz**, with exact octave doubling above it.

This pitch check is deliberately separate from the FM-1 keyboard-layout contract. The physical keyboard passes its real chromatic MIDI note to the ACID engine; Virtual/ReBirth-style C…C entry is a UI convenience and must not retune the physical instrument.

## Fixed-point headroom calibration

Open303 keeps its oscillator, filter and VCA in floating point, so aggressive resonance/accent peaks can temporarily exceed a normalized ±1 signal without immediately becoming a hard clip. The FM-1 path ultimately feeds a bounded Q15 mixer, so the beta.7 oscillator banks reserve **6 dB of source headroom** before the ladder.

That headroom does **not** alter pitch, the cutoff law, resonance mapping, Env Mod mapping, decay, slide or accent timing. It exists to keep the characteristic resonant/accented transient inside the fixed-point operating range instead of flattening it against ±32767. Both compact target tables and the Virtual reference tables use the same headroom policy. The host suite also renders an aggressive low-register resonant/accented patch and rejects sustained rail-clipping.

## Measured Env Mod mapping

Open303 derives the envelope mapping from measured TB-303 behavior. The fixed-point port retains the same constants conceptually:

- nominal cutoff endpoints: 313.815 Hz / 2394.412 Hz;
- envelope offset: `0.294391 + 0.048293 * cutoffPosition`;
- low-cutoff envelope scaler: `0.736966 + 3.773996 * envMod`;
- high-cutoff envelope scaler: `0.864345 + 4.194549 * envMod`;
- the effective scaler is interpolated between low/high values according to the exponential cutoff position.

The effective cutoff is therefore based on `nominalCutoff * 2^(scaler * (envelope - offset) + accentContribution)`, with conservative target bounds applied afterwards. This is intentionally very different from a generic synthesizer's linear `cutoff + envelopeAmount` behavior.

## Resonance model

The resonance knob is skewed with the same musical mapping used by Open303 (`(1-exp(-3r))/(1-exp(-3))`). The target then evaluates the TB-303 coefficient approximation for `b0`, feedback `k`, output gain `g` and the 150 Hz feedback high-pass using fixed-point arithmetic. Beta.7 intentionally leaves these equations unchanged: the reference comparison found no justification for arbitrary resonance boosts or cuts.

## Oscillator shapes

The compact FM-1 tables follow Open303's 303 oscillator construction:

- SAW303 uses the split rising-saw prototype;
- SQUARE303 is derived from that saw with the Open303 tanh shaping defaults (**36.9 dB**, offset **+4.37**), polarity inversion and **180°** phase shift;
- square is retained at half the saw level, matching Open303's BlendOscillator convention;
- pitch bands are band-limited for the final 44.1 kHz output bandwidth while the oscillator/filter run at 4x internally.

The browser's larger HQ bank remains a second spectral reference, but applies the same 6 dB fixed-point headroom policy.

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

## Validation

The core has explicit tests for:

- A4=440 equal temperament and the TB-303 110 Hz low-A service anchor;
- end-to-end rendered pitch at multiple octaves;
- bounded output and aggressive-patch rail occupancy;
- measured cutoff endpoints/midpoint;
- saw vs 303-square producing different audio;
- stock mode keeping LFO disabled;
- slide preserving the running filter envelope;
- accented slide changing accent state without envelope retrigger;
- normal vs accented VCA release behavior;
- raw main-envelope vs 15 ms accent-RC behavior;
- idle-only analog-path reset;
- all LFO shapes advancing and producing different rendered audio;
- deterministic whole-groovebox audio fingerprint.

The browser and FM-1 target remain calibrated from the same 303 reference assumptions. A successful render/build is not by itself proof of exact analog equivalence; the final calibration still requires listening/reference comparison and physical FM-1 CPU/underrun validation.
