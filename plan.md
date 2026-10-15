# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

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

# Current checkpoint — beta.13 303 target-DSP correction, 2026-10-15

## Physical beta.12 result

The important physical result has changed the debugging priority:

- [x] beta.11 direct ~1 kHz target-boundary reference tone was clean;
- [x] after the latest physical install the former added/doubled/scratch-like sound is reported gone;
- [x] overall output now sounds clean;
- [ ] 303 still sounds too close to a clean sine/fundamental;
- [ ] physical 303 Cutoff/Resonance/Env Mod response is missing or far too weak.

The UI/control routing was rechecked:

- physical `K1/K2/K3/K4` on `UI240_ACID_MAIN` directly writes `cutoff/resonance/env_mod/decay`;
- browser/MIDI `CC74/71/1/73` writes the same `acid303_t` fields;
- note events do not reset those controls.

Therefore beta.13 targets the active 303 DSP, not the UI and not the shared mixer.

## Root-cause work for beta.13

The beta.12 303 path still contained many signed `int64_t` operations in the physical realtime path even after the drum/mixer target path had been converted to bounded 32-bit arithmetic. This included:

- wavetable interpolation/crossfade;
- pre/post/feedback high-pass;
- ladder feedback and four stage updates;
- Open303 coefficient polynomial/rational math;
- Env Mod/exponential cutoff mapping;
- VCA/declick/accent calculations.

The host formulas are numerically plausible and structurally match Open303's TB_303 branch, so beta.13 preserves the model while replacing target-hostile arithmetic rather than retuning by ear.

A second concrete bug exists in coefficient smoothing: `(target-current)/32` can truncate to zero when the remaining delta is smaller than 32, so a coefficient can remain permanently short of its target. That behavior is unnecessary because 32 output samples are already less than one millisecond.

## beta.13 implementation

- [x] keep the measured ~314–2394 Hz nominal Cutoff law;
- [x] keep 4× active oscillator/ladder processing at 176.4 kHz;
- [x] keep the beta.9 Open303-derived 12×1024 target wavetable bank;
- [x] replace realtime Open303 coefficient polynomial/division with a compact 65-point 200–20,000 Hz LUT;
- [x] derive the LUT from the beta.12 fixed-point coefficient law, including its conservative high-resonance bound;
- [x] use bounded signed 32-bit Q15 arithmetic for ladder/state multiplications that would overflow a direct 32-bit `a*b`;
- [x] remove `int64_t`/`uint64_t` from `acid303.c`;
- [x] remove 64-bit arithmetic from wavetable interpolation/crossfade;
- [x] remove 64-bit arithmetic from HPF, Env Mod, VCA, accent, LFO and slide hot paths;
- [x] change coefficient updates to exact 32-sample control-rate targets instead of truncating integer slew steps;
- [x] preserve physical MIDI/UI parameter contracts;
- [x] add a rendered-audio regression requiring a material Cutoff MIN→MAX change;
- [x] add live same-note Cutoff-change regression so the filter must respond without a note retrigger;
- [x] add a material Resonance-response regression;
- [x] add a harmonic/edge-motion regression that prevents the open-cutoff C2 SAW from collapsing toward the closed-filter/fundamental-only character;
- [x] add a source regression that rejects 64-bit integer types in `acid303.c`.

## beta.13 validation/release gate

Before publishing `v0.1.0-beta.13`, the exact final `main` SHA must pass:

1. [ ] normal host DSP/unit/integration suite;
2. [ ] new 303 cutoff/resonance/harmonic-response regression;
3. [ ] existing wavetable + PolyBLEP 303 suites;
4. [ ] physical-define target bridge tests;
5. [ ] browser/JS syntax checks;
6. [ ] deterministic Felucca overlay checks;
7. [ ] shared WASM build + browser smoke;
8. [ ] real JieLi pi32v2 app compile/link;
9. [ ] real FM-1 loader/FWSC package;
10. [ ] target XIP/RAM/pool budgets;
11. [ ] ROM-reference safety gate;
12. [ ] manifest + SHA verification;
13. [ ] guarded publisher against the exact successful Build-all SHA;
14. [ ] publish new immutable `v0.1.0-beta.13`; never move beta.12 or earlier tags.

## beta.13 physical acceptance

After the retained beta.12 ten-second startup isolation sequence:

- [ ] former scratch/double component remains absent;
- [ ] C2 SAW is clearly harmonically richer than a clean sine;
- [ ] C2/C3/C4 Cutoff sweep is unmistakable from minimum to maximum;
- [ ] Resonance materially changes the response at medium/high Cutoff;
- [ ] Env Mod materially changes the transient filter sweep;
- [ ] SAW and SQUARE remain audibly distinct;
- [ ] no new aliasing/digital scratch is introduced;
- [ ] capture runtime diagnostics after a sustained ACID pattern.

If the physical filter remains static after this all-32-bit path, beta.14 should stop retuning and instead expose selectable raw-wavetable → pre-HP → ladder → post-HP stages for direct hardware comparison.

---

# beta.12 staged DSP isolation — retained for comparison

Physical-only startup sequence:

1. [x] 0–2 s direct 1 kHz reference;
2. [x] 2–4 s direct 808 BD;
3. [x] 4–6 s mixed 808 BD;
4. [x] 6–8 s direct complete 303 C2 SAW;
5. [x] 8–10 s mixed complete 303 C2 SAW;
6. [x] fresh stopped groovebox after diagnostics;
7. [x] Host/Virtual unaffected.

The probe remains in beta.13 for one controlled A/B physical comparison. It should be removed or made explicitly selectable once the 303 target path is physically approved.

---

# Engine/runtime status

## ACID A / ACID B

- [x] two independent mono instances;
- [x] saw + Open303-derived 303 square;
- [x] A4=440 equal temperament;
- [x] true physical chromatic keyboard mapping;
- [x] nominal 60 ms slide;
- [x] Cutoff/Resonance/Env Mod/Decay/Accent controls;
- [x] 4× oscillator/filter processing;
- [x] 12-band ×1024 target mip bank;
- [x] smooth two-semitone mip transitions;
- [x] idle-voice fast path;
- [x] beta.13 bounded 32-bit active DSP;
- [x] beta.13 exact control-rate coefficient update;
- [ ] physical beta.13 filter approval;
- [ ] final physical calibration only after functional filter response is proven.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] distinct model frequencies/decays;
- [x] open-hat choke;
- [x] persisted 909 flam + width;
- [x] direct hardware audition/programming;
- [x] synthetic fallback without licensed samples;
- [x] signed-32 synthetic target hot path;
- [ ] final physical model/timbre redesign remains open.

## Sequencer / mixer / FX

- [x] 16-step patterns + A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized live pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] separate PCF / Delay / Dist / Comp performance sections;
- [x] neutral startup gain contract;
- [x] signed-32 shared mixer hot path;
- [x] versioned project save/load;
- [ ] longer patterns/ratchets/automation deferred until physical audio baseline is reliable.

---

# Physical UI / keyboard contract

## ACID main

1. `ENV` → ACID A.
2. `LFO` → ACID B.
3. `K1` → Cutoff.
4. `K2` → Resonance.
5. `K3` → Env Mod.
6. `K4` → Decay.
7. Musical keys → direct chromatic live play.

## ACID step

1. `SEQ` → selected ACID step editor.
2. `PRESET` → step 1–16.
3. Musical key → chromatic note entry/audition.
4. `OCT- / OCT+` → register.
5. `K1` → stored-note semitone.
6. `K2` → REST / NOTE / TIE.
7. `K3` → Accent/Slide.
8. `K4` → probability.

## DRUM

1. `ARP` → 808/909 programmer.
2. Contextual/repeated selection switches 808/909.
3. `PRESET` → step 1–16.
4. `K1` or `OCT- / OCT+` → lane.
5. `K2` → OFF / HIT / ACCENT.
6. Musical key → mapped drum select/program/audition.
7. 909 `K3` → FLAM.
8. 909 `K4` → flam width.

## RB-240

- [x] compact page-specific physical renderer;
- [x] selected step vs playhead distinction;
- [x] FOCUS location indicator;
- [x] dirty-region redraw;
- [ ] physical zero-flash/stale-pixel/label-overlap validation.

---

# Shared realtime corrections retained

## Mixer/drums

- [x] bounded signed-32 track level/pan;
- [x] bounded delay/reverb send/feedback;
- [x] DIST=0 neutral startup;
- [x] transparent compressor startup;
- [x] PCF-open bypass;
- [x] -12 dB shared master summing headroom;
- [x] synthetic drum hot path without unnecessary signed-64 operations.

## Output facts

- [x] Felucca's final Q15→24-bit `<<7` conversion remains after ReFm render;
- [x] beta.11 direct target-boundary tone physically clean;
- [x] latest physical result reports former doubled/scratch artifact gone;
- [x] no duplicate output shift/EQ/limiter workaround added.

---

# Browser/release infrastructure

- [x] browser installer/editor/Virtual FM-1;
- [x] CORS-safe firmware channel;
- [x] exact-tag GitHub release verification instead of relying on release-list ordering;
- [x] `.fwsc` asset-name + SHA-256 verification;
- [x] guarded one-shot beta publisher;
- [x] immutable beta tags.

---

# Safety / recovery

- [x] pinned Felucca `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi app/loader/FWSC build;
- [x] package identity/SHA/memory gates;
- [x] ROM-reference safety gate;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] verified official V15 recovery workflow available;
- [x] immutable beta policy.

Still required physically:

- [ ] beta.13 303 cutoff/resonance/env-mod acceptance;
- [ ] factory-pattern runtime capture;
- [ ] worst-case four-machine stress capture;
- [ ] `late == 0` under mixed load;
- [ ] sustained audio CPU <=75%;
- [ ] peak render <=82% of DMA-half deadline;
- [ ] user-stack free >=4096 bytes;
- [ ] supervisor/ISR-stack free >=1024 bytes;
- [ ] USB MIDI/TRS MIDI/USB audio under load;
- [ ] save/load + power-cycle behavior;
- [ ] repeated update/rollback cycle.

No custom firmware has a zero-brick guarantee.
