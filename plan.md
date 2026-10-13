# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

- **ACID A / ACID B** — two independent 303-inspired mono bass synths
- **808 / 909** — two distinct drum-machine workflows
- A–H patterns + song/chain sequencing
- four-channel mixer + performance FX
- USB/TRS MIDI + USB audio
- original ReBirth-inspired 240×240 physical UI
- browser installer/editor + Virtual FM-1/WASM reference
- project persistence and safe recovery workflow

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — post-beta.3 hardware usability work, 2026-10-13

`v0.1.0-beta.3` is the current published hardware-test baseline. It was released only after the exact release commit passed host, WASM and real FM-1 package gates. **Do not overwrite or repoint beta.3.** Current `main` contains unreleased post-beta.3 UI/usability work.

The first physical validation established that installation/boot and basic LCD rendering work, but the firmware is **not yet physically usable as a groovebox**. Reported blockers include:

- unwanted horizontal/pattern graph content bleeding through pages;
- visible whole-screen refreshes while changing parameters;
- overlapping/dense controls and unnecessary bottom help bands;
- STEP/DRUM/MIXER/FX/SONG/PROJECT workflows not practically usable;
- no proven audible output;
- no proven live keyboard play;
- note/sequence editing not practically usable.

These are release blockers. A successful compile/package is not evidence that these physical interaction/audio issues are solved.

## Latest corrective work on `main`

- [x] Virtual FM-1 now reads the **real 16 ACID steps** and **real 16 drum steps** from the WASM groovebox state instead of reconstructing a fake strip from selected-step values.
- [x] Virtual ACID editing is pattern-first/TB-303-style: select a step, then directly choose note/rest/tie/accent/slide.
- [x] Virtual drum editing directly selects a lane and cycles each step through OFF → HIT → ACCENT; 909 flam remains real sequencer state.
- [x] Virtual display supports a true **2× 480×480** pixel-perfect view while keeping the logical firmware display 240×240.
- [x] Old modifier/chord controls remain available for performance, but they are no longer the primary Virtual sequencing workflow.
- [x] Added an original **TheReFmB1rth startup sequence** for Virtual FM-1 and physical FM-1.
- [x] Startup sequence is approximately 1.56 s and progresses through ACID A → ACID B → 808 → 909 → 16-step bus → READY.
- [x] Physical startup animation runs after input/audio/USB/timer initialization and replaces the previous blocking Felucca text delay.
- [x] Removed the legacy Felucca 16-step graph-strip injection from the physical overlay; the dedicated RB-240 renderer now owns the LCD without that compatibility layer.
- [x] Added regression tests for the renderer chain, startup renderer and clean LCD graph ownership.
- [ ] Revalidate the above changes on real FM-1 hardware.

---

# Virtual sequencing contract

The Virtual FM-1 must be usable without memorizing button chords.

## ACID A / ACID B

Primary editing workflow:

1. open **SEQ**;
2. select one of 16 steps;
3. select note, REST or TIE;
4. toggle ACCENT and SLIDE directly;
5. hear the result through the same groovebox runtime used by the target build.

Display requirements:

- [x] selected step is distinct from playback step;
- [x] REST / NOTE / TIE comes from actual `seq16_step_t` flags;
- [x] accent and slide come from actual sequencer state;
- [x] all 16 steps are exposed through the WASM panel ABI;
- [x] direct screen edits update the real engine state.

## 808 / 909

Primary editing workflow:

1. choose 808 or 909;
2. choose BD/SD/CH/etc. lane;
3. click a step to cycle OFF → HIT → ACCENT;
4. 909 flam is shown independently where present.

- [x] actual hit/accent masks are rendered;
- [x] selected lane is explicit;
- [x] selected step and running step are distinct;
- [x] 909 flam remains persisted engine state, not a cosmetic browser flag.

## 2× display

- [x] source display remains 240×240;
- [x] optional 480×480 canvas uses nearest-neighbor/pixel-perfect scaling;
- [x] direct sequence editing also works from the enlarged display.

---

# Startup / identity screen

The startup is inspired by the dense industrial-machine character of classic groovebox software, but uses original TheReFmB1rth branding and assets.

Visual contract:

- brushed/industrial panel language;
- **THE ReFM B1RTH** identity;
- **RB-240** display designation;
- ACID A / ACID B / 808 / 909 machine rows;
- 16-step bus LEDs;
- final READY state;
- no copied Roland, Propellerhead or ReBirth logos/artwork.

Hardware constraints:

- [x] 240×240 logical layout;
- [x] existing LCD/text primitives only;
- [x] no large bitmap animation asset required;
- [x] incremental phase updates rather than a continuous expensive animation loop;
- [x] audio/input/USB initialization completes before animated phases;
- [x] previous extra blocking `fm1_delay_ms(400)` boot delay is removed from the generated target path;
- [ ] verify startup timing and LCD appearance on physical FM-1.

---

# Physical RB-240 cleanup — current priority

## P0: screen ownership and refresh

- [x] remove legacy compatibility graph strip from normal ReFm pages;
- [x] test that the overlay no longer injects `refm_felucca_step` graph content;
- [ ] inspect all generated target pages on device for any remaining Felucca bleed-through;
- [ ] remove unnecessary persistent bottom help bands from physical pages;
- [ ] avoid full 240×240 clears for routine parameter changes;
- [ ] move physical rendering toward dirty-region updates for knobs/values/step cells.

## P0: audible output and live play

Software-side facts already proven by host tests:

- [x] direct MIDI into `refm_felucca_midi_packet()` renders non-zero audio;
- [x] the physical PLAY action starts the ReFm transport;
- [x] seeded Pattern A renders non-zero audio and advances steps;
- [x] ReFm replaces Felucca `mix_block()` in the target audio callback;
- [x] ReFm Q15 output is passed through Felucca's existing DAC output scaling.

Physical-path validation still required:

- [ ] prove target audio ISR is continuously calling `refm_felucca_audio_block()`;
- [ ] prove physical PLAY reaches ReFm transport on device;
- [ ] prove physical key press/release reaches `refm_felucca_key_event()`;
- [ ] prove rendered target block contains non-zero samples on device;
- [ ] if samples are non-zero but headphones/USB remain silent, isolate codec/DAC/UAC handoff rather than changing DSP blindly;
- [ ] validate master-volume behavior on the actual FM-1 output path.

## P0: physical editing

The current modifier-heavy hardware interaction must not be considered finished merely because its state machine has tests.

- [ ] redesign ACID physical step entry around a simple selected-step workflow comparable to the Virtual editor;
- [ ] make PRESET/step navigation predictable without hidden page/layer interactions;
- [ ] make note entry, REST, TIE, ACCENT and SLIDE possible with a short repeatable sequence of controls;
- [ ] simplify 808/909 lane + step programming;
- [ ] ensure labels correspond to the physical encoders/buttons actually used;
- [ ] remove or demote advanced hold/chord functions that interfere with basic sequencing.

---

# Existing engine/runtime features

## ACID A / ACID B

- [x] saw + square;
- [x] tune/pitch;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] slide + tie/rest/note sequencing;
- [x] independent dual instances;
- [x] per-ACID distortion;
- [x] source-note slide semantics;
- [x] fixed-point target engine + WASM HQ reference;
- [ ] compare against real TB-303 reference recordings;
- [ ] physical cutoff/resonance/env-mod calibration;
- [ ] physical square/distortion listening approval.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] synthesized baseline voices;
- [x] independent 808/909 lanes;
- [x] open-hat choke behavior;
- [x] optional sample path + auditable sample tooling;
- [x] real 909 flam state + width;
- [x] immediate drum audition in runtime;
- [ ] physical listening approval;
- [ ] confirm final physical lane order and editing interaction;
- [ ] choose final target sample subset only after physical CPU/XIP measurements.

## Sequencer / song / mixer / FX

- [x] 16-step patterns and A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized manual pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] delay/reverb return processing;
- [x] master Dist → Comp → PCF signal path;
- [x] versioned project save/load;
- [ ] only expose DSP parameters that are actually implemented;
- [ ] defer 32/48/64-step UX, ratchets and automation until the 16-step hardware workflow is solid.

---

# Browser / Virtual FM-1

- [x] INSTALLER / LIVE EDITOR / VIRTUAL FM-1 / VALIDATION combined app;
- [x] physical chassis/key ordering represented in virtual unit;
- [x] full 16-step ACID/drum readback;
- [x] direct-click ACID/drum sequencer editing;
- [x] true 480×480 display mode;
- [x] startup-animation parity with the target concept;
- [x] shared target/WASM runtime and `ui240` state;
- [x] advanced modifier/FOCUS interaction remains available;
- [x] expandable Virtual help/readme;
- [ ] add browser-level DOM automation for direct step editing and pointer timing;
- [ ] final pixel-font/LED polish after real LCD comparison.

---

# Safety / recovery / performance gates

Already implemented:

- [x] pinned Felucca 1.0.5 foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and `.fwsc` generation;
- [x] package identity/SHA and memory gates;
- [x] loader-mediated installer foundation and official-V15 identity checks;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] browser diagnostic capture and physical-validation report workflow;
- [x] guarded one-shot beta publisher;
- [x] `v0.1.0-beta.3` published without overwriting earlier beta tags.

Still required physically:

- [ ] factory-pattern runtime capture;
- [ ] worst-case four-machine stress capture;
- [ ] `late == 0`;
- [ ] sustained audio CPU <= 75%;
- [ ] peak render <= 82% of DMA-half deadline;
- [ ] user-stack free >= 4096 bytes;
- [ ] supervisor/ISR-stack free >= 1024 bytes;
- [ ] USB MIDI, TRS MIDI and USB audio under playback/edit load;
- [ ] save/load and power-cycle behavior;
- [ ] return-to-official/Felucca recovery;
- [ ] repeated update/rollback cycles.

No custom firmware has a zero-brick guarantee.

---

# Release policy

- [x] `main` produces temporary host/WASM/FM-1 artifacts;
- [x] verified one-shot beta publisher exists;
- [x] beta.3 is immutable once published;
- [x] publication guard skips release/channel mutation without an explicit release request;
- [ ] do not publish the current post-beta.3 work until exact-head host + WASM + real FM-1 package gates are green;
- [ ] then test the new startup, clean LCD ownership, audio and basic physical sequencing on hardware;
- [ ] require the basic physical workflow to work before promoting advanced modifier/chord features;
- [ ] publish another hardware beta only after explicit user request.

---

# Immediate next tasks

1. Get the current clean-graph/startup head exact-green in normal CI, WASM and real FM-1 package jobs.
2. On hardware, verify the old horizontal/pattern strip is gone on every page.
3. Capture evidence for the audio chain in order: PLAY/key edge → ReFm event → non-zero render → DAC/USB output.
4. Simplify the physical ACID step editor to match the direct Virtual workflow as closely as the FM-1 controls allow.
5. Simplify physical drum lane/step editing.
6. Replace full-screen redraws during parameter changes with dirty-region updates.
7. Remove unnecessary bottom help labels from physical pages.
8. Re-run physical validation and runtime diagnostics.
9. Only then consider another beta release.

---

# Definition of done for v1.0

- [x] two independent ACID engines;
- [x] 808 + 909 simultaneously available;
- [x] 16-step patterns/song baseline;
- [x] accent/slide/probability;
- [x] mixer + essential FX;
- [x] browser installer/editor/Virtual FM-1/WASM workflow;
- [x] original startup identity and guarded release provenance;
- [ ] physical ACID sequencing is fast and predictable;
- [ ] physical 808/909 sequencing is fast and predictable;
- [ ] live keyboard play and PLAY transport are audibly proven;
- [ ] physical UI has no renderer bleed-through or distracting whole-screen refreshes;
- [ ] physical CPU/stack headroom proven;
- [ ] USB MIDI/TRS MIDI/USB audio validated;
- [ ] save/load validated physically;
- [ ] recovery/return-to-official proven;
- [ ] all final samples have auditable licenses;
- [ ] no known flash/RAM/stack/CPU budget violations.
