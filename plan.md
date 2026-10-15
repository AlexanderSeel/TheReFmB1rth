# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

- ACID A / ACID B — two independent 303-inspired mono synths;
- 808 / 909 — distinct drum-machine workflows;
- A–H patterns + song/chain sequencing;
- four-channel mixer + PCF / Delay / Dist / Comp performance FX;
- USB/TRS MIDI + USB audio;
- original ReBirth-inspired 240×240 physical UI;
- matching browser installer/editor + Virtual FM-1/WASM;
- project persistence and safe recovery workflow.

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — beta.10 shared realtime-audio correction, 2026-10-15

## Physical beta.9 finding

Physical beta.9 does **not** sound like a remaining SQUARE-only wavetable problem. The user reports:

- a dominant high, scratchy/digital sound that feels at least an octave above the intended 303 bassline;
- the actual 303-like bassline is only faintly audible underneath;
- cutoff, resonance, Env Mod and decay therefore appear to have only subtle effect;
- the same high scratch is also present on 808/909 drums.

Because independent synth and drum engines share the artifact, the primary beta.10 hypothesis is the common target realtime/output path, not another isolated oscillator-timbre error.

The next immutable hardware-test prerelease is **`v0.1.0-beta.10`**. beta.9 and older tags/releases must remain untouched.

## What has been ruled out

- [x] Verify that Felucca's original final Q15→24-bit conversion is still executed after the ReFm render.
- [x] Confirm that replacing `mix_block()` did not remove the later UAC/DAC path.
- [x] Confirm target fixed output attenuation remains unity after shared mixer headroom; no duplicate gain stage is being reintroduced.
- [x] Preserve beta.9 wavetable/pitch behavior while investigating the common artifact.

The original Felucca `OUT_SHIFT` remains in the physical path, so beta.10 must not add a second guessed output shift.

---

# beta.10 implementation

## 1. Shared mixer target arithmetic

- [x] Replace repeated signed `int64_t` multiplies in the per-sample mixer hot path with bounded signed-32 fixed-point arithmetic.
- [x] Keep track-level/pan behavior unchanged.
- [x] Keep delay/reverb send behavior unchanged.
- [x] Keep DIST=0 neutral.
- [x] Keep COMP startup transparent.
- [x] Keep PCF-open bypass behavior.
- [x] Preserve the shared -12 dB master summing stage.
- [x] Clamp pre-master accumulation before the final Q15 multiply so the product cannot overflow signed 32 bit.
- [x] Add worst-legal-input polarity and full-send/full-FX state-bound regression coverage.

Important bounds:

- Q15×Q15 products remain below signed 32-bit range;
- the maximum DIST gain product remains below `INT32_MAX`;
- reverb/PCF difference products are explicitly bounded;
- pre-master is clamped before multiplying by the -12 dB Q15 gain.

## 2. Drum hot path

- [x] Remove unnecessary signed-64 multiplications from per-sample synthesized drum oscillator/ring-modulation code.
- [x] Keep 808/909 frequencies, decay laws and model distinctions unchanged.
- [x] Keep signed bipolar hat/cymbal ring synthesis; do not reintroduce XOR-style audio.
- [x] Keep sample-rate conversion setup outside the normal synthetic per-sample path.

## 3. Idle 303 CPU fast path

- [x] Do not run an ACID oscillator + 4× ladder when the voice is fully released, gate-off and `ACID_ENV_OFF`.
- [x] Return exact zero for a fully idle ACID mixer input.
- [x] Preserve the complete existing DSP path as soon as a note/envelope is active.
- [x] This removes two continuously running silent 303 ladders during drum-only playback.

This is a key diagnostic change because beta.9 still consumed expensive 303 DSP time even when only drums were audible.

## 4. Deliberately unchanged for beta.10

- [x] no new SAW/SQUARE shaping;
- [x] no random drift/detune;
- [x] no oscillator EQ workaround;
- [x] no cutoff/resonance/Env Mod retuning;
- [x] no envelope/decay retuning;
- [x] no additional output attenuation;
- [x] no new always-on limiter/distortion;
- [x] keep beta.9 12×1024 physical wavetable bank;
- [x] keep two-semitone mip crossfades;
- [x] keep true 60 ms slide law.

---

# Validation gates for beta.10

The exact release commit must pass:

1. [x] host DSP/unit tests after signed-32 mixer conversion;
2. [x] target mixer stress regression compile under `-Wall -Wextra -Werror`;
3. [x] mixer full-scale positive/negative polarity checks;
4. [x] delay/reverb ring-index bounds under maximum legal sends/feedback;
5. [x] existing 303 pitch, slide, spectral and headroom regressions;
6. [x] existing drum tuning/headroom regressions;
7. [x] deterministic Felucca overlay compile/link test;
8. [x] idle-303 fast path passes existing groovebox integration/persistence tests;
9. [ ] final exact `main` CI green;
10. [ ] final exact `main` WASM build + browser smoke test green;
11. [ ] final exact `main` pinned Felucca overlay verification green;
12. [ ] final exact `main` real JieLi app/loader/FWSC build green;
13. [ ] target XIP/RAM/ROM-reference gates green;
14. [ ] release manifest/checksum verification green;
15. [ ] publish `v0.1.0-beta.10` only from that exact successful `main` commit.

Existing beta tags remain immutable.

---

# Physical beta.10 test order

Use low external/headphone volume first.

1. [ ] STOP state should remain silent.
2. [ ] Play one 808 kick with both ACID voices inactive. Listen specifically for the previous common high scratch.
3. [ ] Repeat with 909 kick/snare and hats.
4. [ ] Play one sustained SAW ACID note with drums stopped.
5. [ ] Repeat on SQUARE.
6. [ ] Sweep cutoff from low to high; the audible main body should clearly track the control rather than only a faint background layer changing.
7. [ ] Raise resonance independently and confirm strong/expected audible effect.
8. [ ] Raise Env Mod independently and confirm the envelope sweep is clearly audible.
9. [ ] Change decay across a wide range and verify obvious envelope-duration change.
10. [ ] Play ACID + drums together and listen for load-dependent crackle/scratch.
11. [ ] If possible, capture runtime diagnostics during drum-only, one-ACID and full four-machine cases.
12. [ ] Required healthy target indicators remain: `late == 0`, sustained audio CPU <=75%, peak render <=82% DMA-half deadline, user-stack free >=4096, supervisor/ISR-stack free >=1024.

## Decision after physical beta.10

If the shared scratch is gone or greatly reduced:

- continue reducing active-303 target cost only where measurements justify it;
- then resume physical cutoff/resonance/Env Mod calibration and drum timbre refinement.

If the scratch remains essentially unchanged on both drums and 303:

1. do **not** retune oscillator/filter parameters again;
2. add a direct target Q15 sine/test-tone mode that bypasses groovebox + mixer;
3. add direct drum output bypassing the mixer;
4. add 303 pre-ladder and post-ladder diagnostic outputs;
5. compare those physical modes to isolate DSP vs Felucca/I²S/DAC postpath;
6. inspect runtime `late`/render-time diagnostics for DMA underrun or sample-repeat behavior;
7. only after that evidence investigate target-specific 303 ladder `int64_t` lowering or the current 4× decimator.

---

# Engine/runtime status

## ACID A / ACID B

- [x] saw + Open303-derived 303 square;
- [x] equal-temperament pitch and A4=440 regression;
- [x] true physical chromatic keyboard mapping;
- [x] 60 ms slide;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] independent dual instances;
- [x] 4× active oscillator/filter processing;
- [x] 12-band/1024-sample target mip bank;
- [x] shared target/Virtual source model;
- [x] smooth mip transitions;
- [x] fully idle voice fast path;
- [ ] physical clean-audio approval;
- [ ] physical cutoff/resonance/env-mod calibration after common scratch is resolved.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] distinct 808/909 model frequencies/decays;
- [x] open-hat choke;
- [x] real persisted 909 flam + width;
- [x] direct hardware audition/programming;
- [x] synthetic fallback without licensed samples;
- [x] signed-32 synthetic hot path for target;
- [ ] physical clean-audio approval;
- [ ] final physical timbre calibration.

## Sequencer / mixer / FX

- [x] 16-step patterns + A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized live pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] separate Dist / Comp / PCF performance processing;
- [x] clean startup gain contract;
- [x] versioned project save/load;
- [x] signed-32 shared mixer hot path;
- [ ] longer patterns/ratchets/automation remain deferred until the physical audio baseline is reliable.

---

# Physical UI / keyboard contract

## ACID

1. `ENV` → ACID A.
2. `LFO` → ACID B.
3. `SEQ` → selected ACID step editor.
4. `PRESET` → step 1–16.
5. Musical key → true chromatic note entry/audition.
6. `OCT- / OCT+` → physical keyboard register.
7. `K1` → stored-note semitone edit.
8. `K2` → REST / NOTE / TIE.
9. `K3` → Accent/Slide state.
10. `K4` → probability.

## DRUM

1. `ARP` → drum programmer.
2. Contextual/repeated selection switches 808/909.
3. `PRESET` → step 1–16.
4. `K1` or `OCT- / OCT+` → lane selection.
5. `K2` → OFF / HIT / ACCENT.
6. Musical key → mapped drum select/program/audition.
7. 909 `K3` → FLAM.
8. 909 `K4` → flam width.

## RB-240

- [x] compact page-specific physical renderer;
- [x] exact ACID and drum step readback;
- [x] selected step vs playhead distinction;
- [x] FOCUS location indicator;
- [x] dirty-region redraw;
- [ ] physical zero-flash/stale-pixel/label-overlap validation remains open.

---

# Safety / recovery

- [x] pinned Felucca `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi app/loader/FWSC build;
- [x] package identity/SHA/memory gates;
- [x] ROM-reference safety gate;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] guarded one-shot beta publisher;
- [x] immutable beta policy.

Still required physically:

- [ ] factory-pattern runtime capture;
- [ ] worst-case four-machine stress capture;
- [ ] USB MIDI/TRS MIDI/USB audio under load;
- [ ] save/load + power-cycle behavior;
- [ ] official/Felucca recovery verification;
- [ ] repeated update/rollback cycle.

No custom firmware has a zero-brick guarantee.

---

# Release policy

- work directly on `main`;
- use few logically grouped commits and consolidate release work where practical;
- Host, WASM and real FM-1 package must be green on the **same exact commit**;
- release marker requests a new immutable prerelease only after the candidate is ready;
- existing tags/releases are never moved, overwritten or reused;
- beta.10 is a physical diagnostic/correction beta, not a claim that the audio issue is solved until hardware confirms it.
