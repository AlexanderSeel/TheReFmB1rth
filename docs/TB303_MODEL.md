# TB-303 model and ReBirth-inspired UI direction

The ACID engine is not treated as a generic mono subtractive synth. The target is the characteristic behavior of an original TB-303 signal path and sequencer interaction, implemented independently for the FM-1's fixed-point DSP constraints.

## Authentic core

The stock-style path models:

- one oscillator with **SAW** and an asymmetric **303-SQUARE** derived-style waveform;
- precomputed band-limited mipmapped oscillator tables on the FM-1 target;
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

The oscillator uses equal temperament with **A4 = 440 Hz** at TUNE center. The host suite checks the service-calibration anchor used for the original TB-303: the low A register resolves to approximately **110 Hz**, with exact octave doubling above it.

The physical keyboard passes its real chromatic MIDI note directly to the ACID engine. Programmed step editing has its own bounded pitch range and must not clamp live hardware notes. The suite also tests that upper MIDI pitches remain monotonic and that `phase_inc` remains unchanged for a full second when slide and LFO are both disabled. There is intentionally **no random pitch drift or analog-detune modulation** in stock mode.

## Slide correction

Beta.8 corrected the earlier pitch-slew implementation to use `44100 * 0.060 = 2646` samples as the stock 60 ms time constant. The regression test checks that after 2646 samples an octave slide retains approximately `1/e` of its original frequency-increment distance. Slide remains legato and does not retrigger the main filter envelope.

## Beta.9 oscillator / wavetable refinement

The physical beta feedback isolated the remaining 303 concern to source timbre, especially SQUARE. The beta.9 work therefore changes the wavetable construction and lookup path rather than adding EQ, detune or distortion.

### Shared Open303-derived source model

Both the target and Virtual generators now start from the same **2048-sample reference prototype**:

- SAW303 uses Open303's split rising-saw construction;
- SQUARE303 is derived from that exact saw via the Open303-style tanh shaper at **36.9 dB**, offset **+4.37**, followed by polarity inversion and a **180° phase shift**;
- the square table keeps the Open303 BlendOscillator convention of half the raw saw-table amplitude;
- source headroom remains 6 dB below full Q15 so the ladder/accent path has fixed-point room.

This removes the previous mismatch where the FM-1 used the Open303-shaped square while the browser/HQ generator used a more ideal odd-harmonic square. Hardware and Virtual can now be compared meaningfully.

### Target table resolution and pitch coverage

The FM-1 bank is now:

- **1024 samples per table** instead of 512;
- **12 pitch bands** instead of 10;
- bands span the supported MIDI register through note 127;
- each band is reconstructed from the Fourier coefficients of the full 2048-sample source prototype rather than first reducing the prototype itself.

Virtual keeps 2048 lookup samples per table but uses the same audible harmonic budget as the target. Its extra samples improve interpolation resolution only; they do not give Virtual a different oscillator spectrum.

### Harmonic budget / alias guard

The final audio rate is 44.1 kHz while the oscillator/filter core runs at 4x internally. The compact embedded downsampler is intentionally conservative, so beta.9 does not populate partials exactly to final Nyquist. Each pitch band is truncated to a maximum partial below **19.845 kHz** (`0.45 * 44100`).

This small guard is intended to reduce audible fold-back from table interpolation, nonlinear ladder processing and the compact four-sample decimation stage. It does not change oscillator fundamental frequency.

### Smooth mip transitions

Beta.8 selected one complete wavetable per pitch band and changed tables abruptly at the threshold. Adjacent bands contain different high harmonics, so a legato slide could change spectrum discontinuously even though phase and pitch were continuous.

Beta.9 adds explicit crossfade-start metadata and phase-aligned runtime blending:

- the next mip level begins blending in **two semitones before** each threshold;
- both tables are read at the exact same oscillator phase;
- the Q15 blend reaches the next band at the threshold;
- outside a transition region only one table lookup is performed.

The intent is to remove pitch-dependent metallic steps without smoothing or modulating the oscillator's actual frequency.

## Fixed-point headroom calibration

Open303 keeps its oscillator, filter and VCA in floating point, so aggressive resonance/accent peaks can temporarily exceed a normalized ±1 signal without immediately becoming a hard clip. The compact oscillator banks therefore retain source headroom before the ladder.

The clean system-level contract from beta.8 remains: the four-track mixer reserves **-12 dB at the master summing stage before any optional nonlinear processing**. Fresh firmware starts with DIST=0, transparent COMP and PCF fully open/bypassed. The physical MASTER pot remains active after the ReFm render, and target output scaling is unity because fixed summing headroom has one owner.

## Measured Env Mod mapping

Open303 derives the envelope mapping from measured TB-303 behavior. The fixed-point port retains the same constants conceptually:

- nominal cutoff endpoints: 313.815 Hz / 2394.412 Hz;
- envelope offset: `0.294391 + 0.048293 * cutoffPosition`;
- low-cutoff envelope scaler: `0.736966 + 3.773996 * envMod`;
- high-cutoff envelope scaler: `0.864345 + 4.194549 * envMod`;
- the effective scaler is interpolated between low/high values according to the exponential cutoff position.

The effective cutoff is therefore based on `nominalCutoff * 2^(scaler * (envelope - offset) + accentContribution)`, with conservative target bounds applied afterwards. This is intentionally very different from a generic synthesizer's linear `cutoff + envelopeAmount` behavior.

## Resonance model

The resonance knob is skewed with the same musical mapping used by Open303 (`(1-exp(-3r))/(1-exp(-3))`). The target then evaluates the TB-303 coefficient approximation for `b0`, feedback `k`, output gain `g` and the 150 Hz feedback high-pass using fixed-point arithmetic. Beta.9 deliberately leaves these equations unchanged so oscillator work can be judged independently from filter retuning.

## LFO extension

An original TB-303 has no user LFO. The existing LFO remains an optional TheReFmB1rth extension and defaults to zero amount in the ReBirth-style browser UI. All four shapes are connected to real filter modulation: sine-like, triangle, saw and square.

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
- shared 2048-sample Open303 source prototype;
- 1024-sample target tables and 12-band full-register coverage;
- decreasing pitch-band harmonic budgets below the 19.845 kHz guard;
- Open303-shaped square fundamental/body rather than an unrelated ideal square;
- two-semitone mip crossfade metadata;
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

The browser and FM-1 target now use the same audible oscillator assumptions. A successful render/build is not by itself proof of exact analog equivalence; the final calibration still requires listening/reference comparison and physical FM-1 CPU/underrun validation.
