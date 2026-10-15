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

# Current checkpoint — beta.12 staged DSP isolation, 2026-10-15

## Physical beta.11 result

The direct beta.11 ~1 kHz target-boundary startup tone was reported **clean** on physical hardware.

Normal audio still has a clearly audible additional/doubled high component:

- 808/909 remain far from the intended character and carry the strange added sound;
- 303 carries the same effect, especially obvious on low notes such as C2 but still present on C4;
- cutoff now has more audible influence, but the doubled/incorrect component remains.

This substantially reduces the likelihood that the core problem is Felucca's final Q15→24-bit conversion, I²S word format, DAC path or a basic DMA/output-buffer problem.

The pinned Felucca source was rechecked as part of this decision: native `events_block()` and synth rendering are inside Felucca's `mix_block()`, and the ReFm physical overlay replaces that `mix_block()` audio call. A simultaneously rendered native Felucca synth is therefore not the leading explanation for physical-key playback.

## beta.12 diagnostic goal

Do not retune 303 or drums yet. First determine whether the unwanted component is introduced:

- inside `drum_machine_process()`;
- inside the active 303 oscillator/ladder path;
- or only after either engine enters the shared ReFm mixer/FX path.

The next immutable test prerelease is `v0.1.0-beta.12`. beta.11 and all older releases remain untouched.

## beta.12 implementation

Physical-target-only startup sequence, five stages × two seconds = ten seconds total:

1. [x] **0–2 s direct 1 kHz reference** at the target boundary; all ReFm engines/mixer bypassed.
2. [x] **2–4 s direct 808 BD** from `drum_machine_process()`; mixer/FX bypassed; retrigger every 0.5 s.
3. [x] **4–6 s mixed 808 BD** with the same engine setup routed through `mixer_fx_process()`.
4. [x] **6–8 s direct 303 C2 SAW** after the complete active 303 oscillator/ladder; mixer/FX bypassed; retrigger every second.
5. [x] **8–10 s mixed 303 C2 SAW** with the same 303 setup routed through `mixer_fx_process()`.
6. [x] Scale direct engine stages to the same approximate -12 dB reference used by the mixer master headroom so comparisons are about timbre/artifact, not loudness.
7. [x] Reinitialize a fresh stopped groovebox after the diagnostic sequence so temporary diagnostic state cannot leak into normal playback.
8. [x] Keep the staged probe behind `REFM_FELUCCA_PLATFORM`; host and Virtual/WASM behavior remains normal.
9. [x] Extend physical-define host regression to cover all five stages, frame accounting, output bounds and return-to-silence.
10. [x] Extend Felucca bridge tests to settle the full ten-second physical diagnostic before normal transport/audio assertions.

## beta.12 physical decision tree

Use low external/headphone volume before power-on.

- [ ] Stage 1 / 0–2 s: reference tone remains clean.
- [ ] Stage 2 / 2–4 s: classify direct 808 BD as clean or contaminated.
- [ ] Stage 3 / 4–6 s: compare the same 808 BD through the shared mixer.
- [ ] Stage 4 / 6–8 s: classify direct post-ladder 303 C2 as clean or contaminated.
- [ ] Stage 5 / 8–10 s: compare the same 303 C2 through the shared mixer.

### Interpretation

- **2 clean, 3 contaminated** → mixer/FX path introduces the drum artifact.
- **2 contaminated** → synthetic drum engine itself is wrong; stop mixer work for the drum branch and rebuild/replace the synthetic 808/909 models.
- **4 clean, 5 contaminated** → mixer/FX path introduces the 303 artifact.
- **4 contaminated** → active 303 oscillator/ladder/fixed-point path is wrong; next diagnostic split must be raw wavetable/pre-ladder vs post-ladder.
- **2 and 4 clean, 3 and 5 clean, but normal playback later contaminated** → investigate groovebox routing/event scheduling/full-load behavior and target runtime timing counters.
- **2 and 4 both contaminated** → likely two engine-level issues or a lower-level compiler/fixed-point primitive shared conceptually but not by the mixer; inspect drum oscillator arithmetic and 303 target `int64_t` lowering separately.

## Next actions after beta.12 feedback

### If mixer is implicated

- [ ] add direct A/B host+target probe around track level/pan and master stage;
- [ ] bypass delay/reverb state even with zero sends to exclude state corruption;
- [ ] bypass PCF/DIST/COMP independently;
- [ ] inspect signed 32-bit multiply/right-shift behavior on pi32v2;
- [ ] compare raw mixer input sample to final Q15 output on target.

### If 303 direct output is implicated

- [ ] add raw 303 wavetable/pre-ladder probe;
- [ ] compare pre-ladder vs each ladder stage/post-ladder;
- [ ] inspect all active-path signed `int64_t` multiplies/divides on the JieLi compiler;
- [ ] inspect 4× oversampling/phase increment and decimation;
- [ ] inspect ladder state rail/clamp occupancy;
- [ ] keep beta.9 wavetable tables unchanged until raw wavetable hardware output is proven wrong.

### If direct 808 is implicated

- [ ] stop incremental timbre tweaks and rebuild the synthetic 808/909 voice models from reference architecture;
- [ ] verify kick pitch-envelope arithmetic separately from steady-state oscillator;
- [ ] verify snare/noise and metallic voice paths independently;
- [ ] compare direct BD-only hardware output against deterministic host render;
- [ ] keep 808 and 909 implementations distinct rather than applying shared pitch multipliers.

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
- [x] beta.11 direct target-boundary tone is physically clean;
- [x] no duplicate guessed output shift or attenuation is added.

---

# Validation gates for beta.12

The exact release commit must pass:

1. [ ] normal host DSP/unit/integration tests;
2. [ ] physical-define five-stage target probe regression;
3. [ ] deterministic audio hash for normal nonphysical rendering unchanged;
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
15. [ ] publish `v0.1.0-beta.12` without modifying any earlier beta tag.

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
- [ ] beta.12 direct post-ladder classification;
- [ ] final physical cutoff/resonance/env-mod calibration after artifact is resolved.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] distinct model frequencies/decays;
- [x] open-hat choke;
- [x] persisted 909 flam + width;
- [x] direct hardware audition/programming;
- [x] synthetic fallback without licensed samples;
- [x] signed-32 synthetic target hot path;
- [ ] physical clean-audio approval;
- [ ] beta.12 direct BD classification;
- [ ] final physical model redesign/calibration after artifact isolation.

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
- [ ] beta.12 direct-vs-mixed hardware classification;
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

- [x] beta.11 direct boot-tone classification: clean;
- [ ] beta.12 staged direct-vs-mixed classification;
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
