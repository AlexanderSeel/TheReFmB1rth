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
- legato slide without main-envelope retrigger, using the stock **60 ms pitch-slew time constant** directly at the 44.1 kHz control/audio rate;
- accented slides changing accent/decay/release behavior without retriggering the main filter envelope;
- phase/filter state reset only after true idle, preserving continuous analog state through a running acid phrase;
- output AC coupling/high-pass behavior calibrated near Open303's **44.486 Hz** pre-ladder and **24.167 Hz** post-ladder stages;
- mild optional bounded per-ACID drive, disabled by default.

The implementation is circuit-informed rather than copied wholesale from Open303 or any commercial emulator. Open303's MIT-licensed source is used as a documented technical reference; the embedded implementation is an allocation-free fixed-point adaptation for the JieLi target. Attribution is recorded in `docs/THIRD_PARTY_DSP.md`.

## Pitch / tuning calibration

The oscillator uses equal temperament with **A4 = 440 Hz** at TUNE center. In addition to the generic A4 arithmetic test, the host suite checks the service-calibration anchor used for the original TB-303: the low A register resolves to approximately **110 Hz**, with exact octave doubling above it.

The physical keyboard passes its real chromatic MIDI note directly to the ACID engine. Programmed step editing has its own bounded pitch range and must not clamp live hardware notes. Beta.8 also tests that upper MIDI pitches remain monotonic and that `phase_inc` remains unchanged for a full second when slide and LFO are both disabled. There is intentionally **no random pitch drift or analog-detune modulation** in stock mode.

## Slide correction

Beta.7 still carried an implementation mistake: the code divided pitch error by `529` on every 44.1 kHz output sample, which is approximately a 12 ms e-fold. That was much faster than the intended 60 ms stock glide and could make sequences sound as if pitch were constantly smearing or retuning.

Beta.8 uses `44100 * 0.060 = 2646` samples as the pitch-slew time constant. The regression test checks that after 2646 samples an octave slide retains approximately `1/e` of its original frequency-increment distance. Slide remains legato and does not retrigger the main filter envelope.

## Fixed-point headroom calibration

Open303 keeps its oscillator, filter and VCA in floating point, so aggressive resonance/accent peaks can temporarily exceed a normalized ±1 signal without immediately becoming a hard clip. The compact oscillator banks therefore retain source headroom before the ladder.

Beta.8 adds the more important system-level correction: the four-track mixer now reserves **-12 dB at the master summing stage before any optional nonlinear processing**. Fresh firmware starts with DIST=0, transparent COMP and PCF fully open/bypassed. This ensures “effects off” is actually a clean linear path rather than a clipped sum that merely happens to have the distortion control at zero.

The physical MASTER pot remains active after the ReFm render. A previous extra fixed target attenuation is removed so fixed summing headroom has a single owner and the hardware path does not stack unexplained gain changes.

## Measured Env Mod mapping

Open303 derives the envelope mapping from measured TB-303 behavior. The fixed-point port retains the same constants conceptually:

- nominal cutoff endpoints: 313.815 Hz / 2394.412 Hz;
- envelope offset: `0.294391 + 0.048293 * cutoffPosition`;
- low-cutoff envelope scaler: `0.736966 + 3.773996 * envMod`;
- high-cutoff envelope scaler: `0.864345 + 4.194549 * envMod`;
- the effective scaler is interpolated between low/high values according to the exponential cutoff position.

The effective cutoff is therefore based on `nominalCutoff * 2^(scaler * (envelope - offset) + accentContribution)`, with conservative target bounds applied afterwards. This is intentionally very different from a generic synthesizer's linear `cutoff + envelopeAmount` behavior.

## Resonance model

The resonance knob is skewed with the same musical mapping used by Open303 (`(1-exp(-3r))/(1-exp(-3))`). The target then evaluates the TB-303 coefficient approximation for `b0`, feedback `k`, output gain `g` and the 150 Hz feedback high-pass using fixed-point arithmetic. The clean-audio correction deliberately leaves these equations unchanged rather than using resonance boosts/cuts to hide gain-path bugs.

## Oscillator shapes

The compact FM-1 tables follow Open303's 303 oscillator construction:

- SAW303 uses the split rising-saw prototype;
- SQUARE303 is derived from that saw with the Open303 tanh shaping defaults (**36.9 dB**, offset **+4.37**), polarity inversion and **180°** phase shift;
- square is retained at half the saw level, matching Open303's BlendOscillator convention;
- pitch bands are band-limited for the final 44.1 kHz output bandwidth while the oscillator/filter run at 4x internally.

The browser's larger HQ bank remains a second spectral reference and follows the same source-headroom policy. Beta.8 intentionally does **not** redesign the square waveform in response to the beta.7 hardware report. The gain, pitch and drum correctness faults are fixed first. If the square remains thin/metallic on the clean physical path, oscillator prototype and ladder interaction will be isolated next.

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
- upper-MIDI monotonic pitch mapping;
- exact live-note routing above C4;
- static oscillator pitch for one second when slide/LFO are disabled;
- end-to-end rendered pitch at multiple octaves;
- true 60 ms slide behavior;
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
- clean four-track summing with effects off;
- deterministic whole-groovebox audio fingerprint.

The browser and FM-1 target remain calibrated from the same 303 reference assumptions. A successful render/build is not by itself proof of exact analog equivalence; the final calibration still requires listening/reference comparison and physical FM-1 CPU/underrun validation.
