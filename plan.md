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

# Current checkpoint — beta.11 direct physical audio-path probe, 2026-10-15

## Why beta.11 exists

Physical beta.9/10 feedback describes one shared high, scratchy/digital overlay on both 303 and 808/909 output. The intended bass/drum signal remains audible underneath, while cutoff, resonance, Env Mod and decay appear to affect mainly that quieter underlying signal.

Because independent sound engines share the artifact, beta.11 must answer one question before any further synthesis tuning:

**Does the artifact already exist after bypassing the entire ReFm DSP/groovebox/mixer?**

The next immutable test prerelease is `v0.1.0-beta.11`. beta.10 and all older releases remain untouched.

## beta.11 diagnostic implementation

- [x] Add a physical-target-only boot audio probe guarded by `REFM_FELUCCA_PLATFORM`.
- [x] Generate a direct ~1 kHz Q15 test tone at the `refm_target_render_q15()` boundary.
- [x] Keep probe level near quarter-scale (~-12 dBFS) for conservative first listening.
- [x] Output identical left/right samples.
- [x] Bypass ACID A/B, 808/909, groovebox transport rendering, mixer and FX while the probe is active.
- [x] Run probe for exactly 3 seconds / 132300 frames at 44.1 kHz.
- [x] Return automatically to normal beta.10 rendering when the frame counter reaches zero.
- [x] Do not enable the probe in normal host or Virtual/WASM builds.
- [x] Add a dedicated host compile/test with `-DREFM_FELUCCA_PLATFORM=1`.
- [x] Verify probe stereo equality, amplitude bounds, frame countdown and automatic return to stopped-groovebox silence.

The diagnostic oscillator uses bounded signed-32 arithmetic; it does not introduce a new `int64_t` realtime path.

## beta.11 physical decision tree

Use low external/headphone volume before power-on.

1. [ ] Power-cycle the FM-1 with beta.11.
2. [ ] Listen only to the first ~3 seconds.
3. [ ] Classify the direct 1 kHz probe:
   - **CLEAN** — no high scratch / octave-like overlay;
   - **CONTAMINATED** — same scratch/overlay as 303/drums.
4. [ ] After the probe ends, confirm stopped output becomes silent.
5. [ ] Press PLAY and compare 808/909-only output.
6. [ ] Compare one ACID SAW note, then SQUARE.
7. [ ] Sweep cutoff full-range.
8. [ ] Raise resonance independently.
9. [ ] Raise Env Mod independently.
10. [ ] Change decay across a wide range.
11. [ ] Test ACID + drums together.

### If the direct boot probe is contaminated

Do **not** retune oscillator/filter/drums. Next investigation is entirely downstream of ReFm synthesis:

- [ ] inspect Felucca post-render `audio.c` path after the ReFm callback;
- [ ] inspect Q15 → 24-bit conversion and sign extension on the actual target compiler;
- [ ] inspect interleaved stereo layout and I²S/DAC word format;
- [ ] capture/check DMA half-buffer timing and repeated/stale samples;
- [ ] compare USB audio vs physical DAC output if possible;
- [ ] add a deterministic alternating/impulse pattern if needed to detect word alignment or channel stride errors.

### If the direct boot probe is clean but normal engine audio is contaminated

The fault is inside ReFm rendering. Next diagnostic build should provide isolated stems:

- [ ] direct 808 output bypassing mixer/FX;
- [ ] direct 909 output bypassing mixer/FX;
- [ ] ACID oscillator/pre-ladder output;
- [ ] ACID post-ladder/pre-mixer output;
- [ ] normal post-mixer output;
- [ ] target runtime `late`/render-time diagnostics for each path.

Only after those comparisons should 303 ladder Q-format, 4× decimation or drum synthesis be changed again.

---

# beta.10 shared realtime correction retained

## Shared mixer target arithmetic

- [x] bounded signed-32 fixed-point track level/pan;
- [x] bounded delay/reverb sends and feedback;
- [x] DIST=0 neutral;
- [x] COMP startup transparent;
- [x] PCF-open bypass behavior preserved;
- [x] -12 dB shared master summing stage preserved;
- [x] pre-master clamp keeps final Q15 multiply below `INT32_MAX`;
- [x] full-scale polarity/full-send/full-FX regression coverage.

## Drum hot path

- [x] remove unnecessary signed-64 per-sample oscillator/ring operations;
- [x] preserve 808/909 model frequencies and decays;
- [x] preserve bipolar ring/noise behavior;
- [x] keep sample-rate conversion setup outside normal synthetic hot path.

## Idle 303 fast path

- [x] fully released/gate-off `ACID_ENV_OFF` voice returns exact zero;
- [x] do not run oscillator + 4× ladder for an idle voice;
- [x] active voice DSP remains unchanged;
- [x] drum-only playback no longer spends target CPU on two silent 303 ladders.

## Output facts already verified

- [x] Felucca's final Q15 → 24-bit `<< 7` conversion still occurs after ReFm render;
- [x] replacing `mix_block()` did not remove the later UAC/DAC path;
- [x] target fixed attenuation remains unity after shared mixer headroom;
- [x] no duplicate guessed output shift or attenuation is added.

---

# Validation gates for beta.11

The exact release commit must pass:

1. [ ] normal host DSP/unit/integration tests;
2. [ ] physical-define host target probe regression;
3. [ ] deterministic audio hash for normal nonphysical rendering unchanged unless a justified shared-DSP change occurs;
4. [ ] browser/JS syntax and sample-manifest checks;
5. [ ] pinned Felucca integration-anchor checks;
6. [ ] deterministic overlay application;
7. [ ] copied ReFm target source compile/link;
8. [ ] WASM build + browser smoke test;
9. [ ] real JieLi pi32v2 app/loader/FWSC build;
10. [ ] target XIP/RAM/pool budgets;
11. [ ] ROM-reference safety gate;
12. [ ] release manifest + checksums;
13. [ ] sample link-size experiment;
14. [ ] guarded publisher on exact successful Build-all commit;
15. [ ] publish `v0.1.0-beta.11` without modifying any earlier beta tag.

---

# Engine/runtime status

## ACID A / ACID B

- [x] saw + Open303-derived 303 square;
- [x] A4=440 equal-temperament pitch regression;
- [x] true physical chromatic keyboard mapping;
- [x] nominal 60 ms slide;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] dual independent instances;
- [x] 4× active oscillator/filter processing;
- [x] 12-band × 1024-sample target mip bank;
- [x] shared target/Virtual source model;
- [x] smooth two-semitone mip transitions;
- [x] idle-voice fast path;
- [ ] physical clean-audio approval;
- [ ] final physical cutoff/resonance/env-mod calibration after shared artifact is resolved.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] distinct model frequencies/decays;
- [x] open-hat choke;
- [x] persisted 909 flam + width;
- [x] direct hardware audition/programming;
- [x] synthetic fallback without licensed samples;
- [x] signed-32 synthetic target hot path;
- [ ] physical clean-audio approval;
- [ ] final physical timbre calibration.

## Sequencer / mixer / FX

- [x] 16-step patterns + A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized live pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] separate PCF / Delay / Dist / Comp performance sections;
- [x] clean startup gain contract outside diagnostic probe;
- [x] versioned project save/load;
- [x] signed-32 shared mixer hot path;
- [ ] longer patterns/ratchets/automation deferred until physical audio baseline is reliable.

---

# Physical UI / keyboard contract

## ACID

1. `ENV` → ACID A.
2. `LFO` → ACID B.
3. `SEQ` → selected ACID step editor.
4. `PRESET` → step 1–16.
5. Musical key → chromatic note entry/audition.
6. `OCT- / OCT+` → keyboard register.
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
- [x] exact ACID/drum step readback;
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

- [ ] beta.11 boot-probe classification;
- [ ] factory-pattern runtime capture;
- [ ] worst-case four-machine stress capture;
- [ ] `late == 0` under mixed load;
- [ ] sustained audio CPU <= 75%;
- [ ] peak render <= 82% of DMA-half deadline;
- [ ] user-stack free >= 4096 bytes;
- [ ] supervisor/ISR-stack free >= 1024 bytes;
- [ ] USB MIDI/TRS MIDI/USB audio under load;
- [ ] save/load + power-cycle behavior;
- [ ] official/Felucca recovery verification;
- [ ] repeated update/rollback cycle.

No custom firmware has a zero-brick guarantee.
