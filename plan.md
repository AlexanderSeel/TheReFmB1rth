# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

- **ACID A / ACID B** — two independent 303-inspired mono bass synths
- **808 / 909** — distinct drum-machine workflows
- A–H patterns + song/chain sequencing
- four-channel mixer + performance FX
- USB/TRS MIDI + USB audio
- original ReBirth-inspired 240×240 physical UI
- browser installer/editor + Virtual FM-1/WASM reference
- project persistence and safe recovery workflow

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — beta.9 303 oscillator refinement, 2026-10-15

Beta.8 corrected the global physical audio path: exact live pitch routing, stable DDS pitch, true 60 ms slide, clean four-track summing, unity target output after the physical MASTER control, and rebuilt synthesized 808/909 voices.

The remaining physical concern is specifically the 303 source timbre, especially SQUARE sounding thin/metallic. Beta.9 therefore isolates and improves **wavetable generation and mip selection** without changing the already-validated filter, resonance, envelope, accent or mixer laws.

The next requested immutable hardware-test prerelease is **`v0.1.0-beta.9`**. Existing beta tags must not be overwritten or repointed.

## beta.9 wavetable findings

- [x] Beta.8 target tables were only 512 samples per band.
- [x] Beta.8 changed complete mip tables abruptly at pitch thresholds; adjacent bands have different upper partials, so a slide could change spectrum discontinuously.
- [x] Target and Virtual did not use exactly the same source model: target SQUARE was Open303-shaped while the HQ/browser generator used a more ideal square construction.
- [x] The old 10-band target bank did not describe the complete MIDI register as explicitly as the new full-range scheme.

## beta.9 implementation

### Shared SAW303 / SQUARE303 source

- [x] Add one shared generator module for target and Virtual.
- [x] Start from a **2048-sample Open303-style SAW303 prototype**.
- [x] Derive SQUARE303 from that saw using the Open303-style 36.9 dB tanh drive, +4.37 offset, polarity inversion and 180° phase shift.
- [x] Preserve Open303's half-square raw amplitude convention.
- [x] Preserve the existing 6 dB fixed-point source headroom.
- [x] Remove the different ideal-square HQ/browser construction.

### Target table quality

- [x] Increase physical FM-1 tables from **512 to 1024 samples**.
- [x] Increase from 10 to **12 pitch bands**.
- [x] Cover the supported MIDI range through note 127.
- [x] Reconstruct each target band from coefficients measured from the full 2048-sample reference prototype rather than from a reduced prototype.
- [x] Apply a **19.845 kHz** final-output harmonic guard below 22.05 kHz Nyquist.
- [x] Verify harmonic budgets decrease monotonically as pitch rises.
- [x] Keep source peaks away from Q15 rails.

### Smooth mip transitions

- [x] Generate explicit transition-start increments for every adjacent band.
- [x] Start the next band's contribution two semitones before each threshold.
- [x] Read both mip tables at the same oscillator phase.
- [x] Q15-crossfade current→next band through the transition region.
- [x] Use only one lookup when outside a crossfade region.
- [x] Preserve phase increment/pitch; spectral interpolation must not become pitch modulation.

### Virtual parity

- [x] Keep Virtual table lookup resolution at 2048 samples.
- [x] Use the same source prototype, band thresholds, harmonic budgets, source headroom and SAW/SQUARE level relation as hardware.
- [x] Extra Virtual table samples improve interpolation resolution only; they do not add harmonics unavailable on the physical target.

## What beta.9 deliberately does not change

- no random oscillator drift;
- no analog-detune modulation;
- no arbitrary EQ;
- no always-on distortion;
- no resonance retuning;
- no Env Mod retuning;
- no accent retuning;
- no change to the true 60 ms slide law;
- no change to the beta.8 clean mixer/drum baseline.

---

# Simple physical control contract

## ACID

1. `ENV` → ACID A.
2. `LFO` → ACID B.
3. `SEQ` → selected ACID step editor.
4. `PRESET` → select step 1–16.
5. Musical key → write/audition that physical key's true chromatic note.
6. `OCT- / OCT+` → keyboard octave down/up on ACID STEP.
7. `K1` → stored-note semitone down/up.
8. `K2` → REST / NOTE / TIE.
9. `K3` → Accent/Slide state.
10. `K4` → probability.

## DRUM

1. `ARP` → drum programmer.
2. Contextual/repeated drum selection switches 808/909.
3. `PRESET` → select step 1–16.
4. `K1` or `OCT- / OCT+` → lane down/up.
5. `K2` → OFF / HIT / ACCENT.
6. Musical key → select/program/audition mapped drum lane.
7. 909 `K3` → flam toggle.
8. 909 `K4` → flam width.

## Other primary pages

- `HOME` → home/status.
- `GLO` → mixer.
- `FX` → effects.
- `SCL` → song/pattern.
- `SAVE` → project.
- `PLAY` → transport immediately.

---

# Display contract

## Virtual

- [x] real 16-step ACID/drum state;
- [x] selected step distinct from playback step;
- [x] direct screen editing;
- [x] 240×240 source + true 480×480 2× view;
- [x] ReBirth-style C…C pitch workflow remains Virtual-only convenience behavior.

## Physical RB-240 renderer

- [x] no legacy Felucca graph strip under normal pages;
- [x] no persistent help footer;
- [x] compact page-specific layout;
- [x] exact ACID NOTE / REST / TIE / ACCENT / SLIDE readback;
- [x] exact DRUM OFF / HIT / ACCENT / 909 FLAM readback;
- [x] selected step and playhead are independent;
- [x] dirty card/cell redraw for same-page changes;
- [x] full-screen clear only on page/machine transition;
- [ ] verify zero visible full-screen flash during continuous knob rotation on real LCD;
- [ ] verify no clipped/overlapping labels on real LCD;
- [ ] verify no stale pixels remain when values shrink/change.

---

# Engine/runtime features

## ACID A / ACID B

- [x] saw + Open303-derived 303-square;
- [x] tune/pitch;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] slide + tie/rest/note sequencing;
- [x] independent dual instances;
- [x] optional per-ACID drive;
- [x] fixed-point target engine;
- [x] Virtual/WASM reference path;
- [x] Open303/TeeBee coefficient/reference audit;
- [x] service-manual pitch anchor regression test;
- [x] static-pitch-without-modulation regression test;
- [x] correct 60 ms slide regression test;
- [x] aggressive fixed-point headroom regression test;
- [x] shared 2048-sample source-model generator;
- [x] 1024-sample / 12-band physical mip bank;
- [x] final-Nyquist harmonic guard;
- [x] phase-aligned two-semitone mip crossfade;
- [x] target/Virtual oscillator spectrum parity;
- [ ] physical SAW/SQUARE timbre approval on beta.9;
- [ ] physical cutoff/resonance/env-mod listening calibration.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] independent 808/909 lane maps;
- [x] model-specific synthetic frequency/decay baseline;
- [x] open-hat choke;
- [x] real 909 flam state + width;
- [x] direct runtime drum audition;
- [x] synthesized fallback audibility test with no samples;
- [x] drum tuning/headroom regression tests;
- [ ] physical listening approval;
- [ ] physical lane/edit workflow approval.

## Sequencer / song / mixer / FX

- [x] 16-step patterns + A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized live pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] delay/reverb returns;
- [x] Dist → Comp → PCF master performance rack;
- [x] clean/linear default master contract;
- [x] versioned project save/load;
- [ ] defer longer patterns/ratchets/automation until physical basics are approved.

---

# Validation gates for beta.9

The exact candidate commit must pass all of the following before publication:

1. [x] generated target wavetable freshness check;
2. [x] shared-source spectral/generator regression tests;
3. [x] 1024-sample target / 12-band / full-MIDI coverage tests;
4. [x] harmonic-budget and 19.845 kHz guard tests;
5. [x] SQUARE fundamental/body regression against the shared Open303-style source;
6. [x] two-semitone transition metadata tests;
7. [x] A4=440 + low-A ~110 Hz + octave-ratio pitch tests;
8. [x] static live-pitch/no-hidden-drift test;
9. [x] 60 ms slide law test;
10. [x] end-to-end rendered C2/C3/C4 pitch tests;
11. [x] source/headroom/rail tests;
12. [x] deterministic whole-groovebox render fingerprint updated only after behavioral tests passed;
13. [ ] final exact `main` CI overlay/link test green;
14. [ ] final exact `main` WASM/HQ build and browser smoke tests green;
15. [ ] final exact `main` pinned Felucca overlay verification green;
16. [ ] final exact `main` real JieLi FM-1 compile/link/package build green;
17. [ ] final target ROM-reference and memory gates green with safe XIP headroom after larger wavetable bank;
18. [ ] release manifest/checksum verification green;
19. [ ] publish `v0.1.0-beta.9` only from that exact successful `main` commit.

Existing beta tags remain immutable.

---

# Physical beta.9 test order

Test at low external/headphone volume first. This pass is specifically for oscillator character and smoothness.

1. [ ] Hold one SAW note for several seconds with LFO/slide off: pitch must be completely stable.
2. [ ] Hold the same note on SQUARE: pitch must remain identical and stable.
3. [ ] Compare SAW vs SQUARE with resonance, Env Mod, accent and drive low/neutral; SQUARE should no longer present as a thin digital/metallic substitute waveform.
4. [ ] Test low, middle and high keyboard registers and listen for sudden spectral jumps between neighboring notes.
5. [ ] Slowly play chromatic notes across mip-band boundaries; no step-like timbre switch should be obvious.
6. [ ] Program slow slides spanning several semitones and an octave; listen specifically for metallic spectral clicks/changes during the glide.
7. [ ] Repeat the slides with SQUARE and SAW separately.
8. [ ] Raise cutoff while resonance remains low to expose oscillator upper harmonics; check that high notes do not alias into unrelated lower tones.
9. [ ] Raise resonance gradually and verify any metallic component follows the intended ladder resonance rather than appearing in the raw square source.
10. [ ] Compare Virtual and physical at matched note/waveform/settings; their basic SAW/SQUARE spectral identity should now be recognizably the same.
11. [ ] Confirm beta.8 drum and clean-mixer behavior did not regress.
12. [ ] Capture runtime diagnostics if physical pitch/timbre still warbles under sustained four-machine load; then investigate underrun/CPU rather than adding oscillator drift.

---

# Safety / recovery

- [x] pinned Felucca foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and `.fwsc` generation;
- [x] package identity/SHA/memory gates;
- [x] strict ROM-reference safety gate;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] guarded one-shot beta publisher;
- [x] existing beta tags remain immutable.

Still required physically:

- [ ] factory-pattern runtime capture;
- [ ] worst-case four-machine stress capture;
- [ ] `late == 0`;
- [ ] sustained audio CPU <= 75%;
- [ ] peak render <= 82% of DMA-half deadline;
- [ ] user-stack free >= 4096 bytes;
- [ ] supervisor/ISR-stack free >= 1024 bytes;
- [ ] USB MIDI, TRS MIDI and USB audio under load;
- [ ] save/load + power-cycle behavior;
- [ ] recovery to official/Felucca firmware;
- [ ] repeated update/rollback cycle.

No custom firmware has a zero-brick guarantee.

---

# Release policy

- [x] `main` builds host/WASM/FM-1 artifacts;
- [x] verified one-shot prerelease publisher exists;
- [x] publisher requires the exact successful `main` Build-all commit;
- [x] publication is skipped when the requested tag already exists;
- [x] existing betas are immutable;
- [ ] beta.9 publish only after exact candidate commit is green;
- [ ] physical validation after beta.9 remains required before treating it as stable.

---

# Immediate next tasks after beta.9

1. Perform the physical oscillator-focused beta.9 test order above.
2. If raw SQUARE remains too metallic with neutral filter settings, compare a captured physical single-note spectrum against the shared generated reference before changing the ladder.
3. If the source is correct but resonance introduces the unwanted character, isolate fixed-point ladder state saturation/coefficient quantization next.
4. If timbre changes only during slides, capture which pitch boundary is audible and inspect crossfade/target CPU behavior.
5. Resume broader workflow/UI features only after the physical audio baseline is acceptable.
