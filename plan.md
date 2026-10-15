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

# Current checkpoint — beta.8 clean-audio correction, 2026-10-15

The physical beta.7 listening pass failed the audio-quality gate. Reported symptoms were global overload/distortion, a thin metallic 303 square, apparently drifting/detuned pitch and harsh overloaded drums. These are treated as implementation defects, not as a request for more distortion or “analog character”.

The next requested immutable hardware-test prerelease is **`v0.1.0-beta.8`**. Existing beta tags must not be overwritten or repointed.

## Root causes found

- [x] Live ACID MIDI/physical notes were passed through an editor clamp ending at MIDI 60, so every live key above C4 became C4.
- [x] ACID slide claimed the Open303 60 ms behavior but used a ~12 ms per-sample slew divisor.
- [x] Upper MIDI DDS conversion capped the octave calculation and could fold high notes.
- [x] The four-track mixer could narrow/clip the summed bus even with DIST=0.
- [x] Fresh projects still passed through PCF math rather than an explicit fully-open bypass.
- [x] The target stack applied an additional fixed post-master attenuation even after shared mixer headroom was introduced.
- [x] The synthetic 808 kick fundamental was around 390 Hz with an extremely short decay instead of a bass-drum fundamental.
- [x] Several synthetic drum voices were similarly pitched too high.
- [x] Closed/open hats used signed bitwise XOR as an audio-combination operation, producing digital metallic artefacts.

## beta.8 corrective implementation

### ACID pitch / 303 behavior

- [x] Live MIDI/physical note entry reaches `acid303_set_note()` unchanged.
- [x] Programmed-step editing remains independently constrained to its intended C1..C6 editor range.
- [x] DDS remains monotonic through the upper MIDI range instead of folding at an octave cap.
- [x] A4=440 Hz and the TB-303 low-A service anchor near 110 Hz remain explicit tests.
- [x] End-to-end rendered C2/C3/C4 pitch remains guarded.
- [x] A one-second no-slide/no-LFO test proves `phase_inc` and LFO phase remain static: no hidden pitch drift is introduced.
- [x] Stock slide now uses a real 60 ms time constant at 44.1 kHz (`2646` samples per e-fold).
- [x] Existing Open303/TeeBee cutoff, Env Mod, resonance, accent and AC-coupling laws are retained rather than randomly retuned.
- [x] Per-ACID drive remains optional and starts at zero.

### Mixer / clean signal path

- [x] All four track faders start at unity.
- [x] One explicit -12 dB master summing stage provides enough headroom for four full-scale tracks.
- [x] Master DIST starts at zero and is mathematically neutral at zero.
- [x] COMP starts transparent.
- [x] PCF starts fully open and bypasses the one-pole coloration at its maximum setting.
- [x] Four simultaneous hot tracks are regression-tested to remain off the numerical rails.
- [x] Physical MASTER remains active after the ReFm render.
- [x] The obsolete second fixed target attenuation is removed; shared mixer headroom is the single owner of fixed summing attenuation.

### 808 / 909 baseline

- [x] 808 kick fundamental corrected to ~55 Hz.
- [x] 909 kick fundamental corrected to ~60 Hz.
- [x] Kick decay extended so it has a body rather than a short high-pitched click.
- [x] 808/909 snare shell modes moved to approximately 180/200 Hz plus noise.
- [x] Low/mid/high tom fundamentals moved into musically useful ranges.
- [x] Hats use bipolar ring multiplication instead of signed bitwise XOR.
- [x] Cymbal/ride synthesis uses ring/square/noise partials without the old XOR artefact.
- [x] Drum-machine output reserves transient headroom before returning int16.
- [x] Kick+snare+hat simultaneous-hit regression test rejects rail clipping.
- [x] Optional sample routing remains available, but firmware synthesis remains the guaranteed no-sample fallback.

## What is deliberately *not* being done in beta.8

- no random oscillator drift;
- no “analog detune” modulation;
- no always-on distortion;
- no resonance-dependent generic waveshaper;
- no arbitrary EQ to hide gain errors;
- no further square-wave redesign until the corrected physical signal path is heard on hardware.

If the square still sounds metallic after beta.8, the next investigation must isolate oscillator prototype vs ladder behavior on the physical target rather than compensate blindly.

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

- [x] saw + 303-square;
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
- [ ] physical square/saw timbre approval on beta.8;
- [ ] physical cutoff/resonance/env-mod listening calibration;

## 808 / 909

- [x] 11 logical voices per machine;
- [x] independent 808/909 lane maps;
- [x] model-specific synthetic frequency/decay baseline;
- [x] open-hat choke;
- [x] real 909 flam state + width;
- [x] direct runtime drum audition;
- [x] synthesized fallback audibility test with no samples;
- [x] drum tuning/headroom regression tests;
- [ ] physical listening approval on beta.8;
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

# Validation gates for beta.8

The exact candidate commit must pass all of the following before publication:

1. [x] host DSP/integration suite on the corrected branch candidate;
2. [x] A4=440 + low-A ~110 Hz + octave-ratio pitch tests;
3. [x] static live-pitch/no-hidden-drift test;
4. [x] 60 ms slide law test;
5. [x] end-to-end rendered C2/C3/C4 pitch tests;
6. [x] four-track clean-mixer no-rail test;
7. [x] 808/909 fundamental/decay/hat/headroom tests;
8. [x] deterministic whole-groovebox render fingerprint updated only after the behavioral tests passed;
9. [ ] exact final-candidate WASM/HQ build and browser smoke tests;
10. [ ] exact final-candidate pinned Felucca overlay verification;
11. [ ] exact final-candidate real JieLi FM-1 compile/link/package build;
12. [ ] exact final-candidate target ROM-reference and memory gates;
13. [ ] exact final-candidate release manifest/checksum verification;
14. [ ] exact final `main` Build-all green after squashing the validated tree;
15. [ ] publish `v0.1.0-beta.8` only from that exact successful `main` commit.

Existing beta tags remain immutable.

---

# Physical beta.8 test order

Test at low external/headphone volume first. This pass is about correctness and cleanliness before fine timbre matching.

1. [ ] STOP is silent.
2. [ ] MASTER minimum is effectively silent and scales smoothly upward.
3. [ ] Play single live ACID notes across the physical keyboard; every key must produce its own stable pitch, including notes above C4.
4. [ ] Hold one note for several seconds with LFO/slide off; pitch must not wander or continuously retune.
5. [ ] Compare octave-spaced notes for exact octave relationships.
6. [ ] Audition SAW with resonance/Env Mod low; confirm it is clean rather than globally overdriven.
7. [ ] Audition SQUARE with resonance/Env Mod low; note whether any metallic/thin character remains after the gain-path fix.
8. [ ] Increase resonance and Env Mod progressively; verify harshness appears only as an intentional parameter result, not at neutral settings.
9. [ ] Test a programmed slide and confirm it is smooth rather than a rapid pitch smear.
10. [ ] Audition 808 BD alone: it should have a low bass body rather than a high metallic click.
11. [ ] Audition 909 BD and SD alone.
12. [ ] Audition CH/OH; they should be bipolar metallic/noisy hats rather than digital XOR buzz.
13. [ ] Play kick+snare+hat together and then a mixed ACID+drum pattern; no global overloaded sound should appear at neutral DIST/COMP/PCF settings.
14. [ ] Verify DIST can deliberately add coloration when increased and returns to clean behavior at zero.
15. [ ] Capture runtime diagnostics if any warble/dropout remains; then investigate DMA/CPU underrun rather than changing oscillator pitch math.

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
- [ ] beta.8 publish only after exact candidate commit is green;
- [ ] physical validation after beta.8 remains required before treating it as stable.

---

# Immediate next tasks after beta.8

1. Perform the physical clean-audio test order above.
2. If pitch still warbles with slide/LFO off, capture runtime DMA/CPU diagnostics before changing synthesis math again.
3. If pitch is stable but SQUARE remains too metallic, isolate oscillator prototype versus ladder/filter behavior using matched SAW/SQUARE single-note captures.
4. Refine 808/909 envelopes/spectral character only after their fundamental tuning and clean headroom are confirmed on hardware.
5. Resume broader workflow/UI features only after the audio baseline is physically acceptable.
