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

`v0.1.0-beta.3` remains the current published hardware-test baseline. **Do not overwrite or repoint beta.3.** Current `main` is unreleased post-beta.3 work.

The first hardware validation proved install/boot and basic LCD rendering, but also exposed real blockers: stray renderer content, visible full-screen refreshes, wasted footer space, confusing controls, unusable sequence editing and no proven audible/live-play path. Those findings remain the acceptance criteria for the next physical test.

## Implemented corrective work on `main`

- [x] Virtual FM-1 reads the real 16 ACID steps and real 16 drum steps from the groovebox state.
- [x] Virtual ACID editing is direct/TB-303-style: step → note/rest/tie → accent/slide.
- [x] Virtual drum editing is direct: lane → step → OFF/HIT/ACCENT, with real 909 flam.
- [x] Virtual display has a true pixel-perfect **480×480 DISPLAY 2×** mode.
- [x] Added original TheReFmB1rth startup animation: ACID A → ACID B → 808 → 909 → 16-step bus → READY.
- [x] Physical startup phases run after audio/input/USB/timer initialization and remove the previous additional blocking Felucca splash delay.
- [x] Removed the legacy Felucca graph-strip injection that caused the stray horizontal pattern line.
- [x] Removed the persistent 28 px physical help footer.
- [x] Physical renderer now performs a full 240×240 clear only on page/machine transitions, not on routine same-page parameter changes.
- [x] Physical primary buttons are now **immediate page-first controls**; deferred tap-vs-hold modifier ownership is no longer used on hardware.
- [x] Advanced modifier/chord shortcuts remain available only as optional Virtual performance controls.
- [x] Physical SEQ page accepts direct musical-key note entry into the selected ACID step.
- [x] Physical DRUM page accepts direct mapped-key drum programming/audition at the selected step.
- [x] Physical renderer now reads the **exact 16-step pattern window** from the same groovebox state as Virtual:
  - ACID REST / NOTE / TIE / ACCENT / SLIDE;
  - DRUM OFF / HIT / ACCENT;
  - 909 flam;
  - independent selected-step and playhead indicators.
- [x] Runtime diagnostic protocol v2 distinguishes panel input, transport, ReFm rendering and non-zero engine output before the later DAC/UAC path.
- [x] Regression tests cover startup renderer, clean graph ownership, physical LCD cleanup, simple hardware controls, exact physical pattern readback and diagnostics v2.
- [ ] Revalidate all of this on real FM-1 hardware.

---

# Simple sequencing contract

## Virtual ACID

1. Open **SEQ**.
2. Select one of 16 steps.
3. Choose note, REST or TIE.
4. Toggle ACCENT / SLIDE.
5. Play through the same groovebox runtime used by the target build.

## Physical ACID

Primary workflow is now page-first, without hidden holds:

1. `ENV` selects ACID A or `LFO` selects ACID B.
2. `SEQ` opens the step editor immediately.
3. `PRESET` selects step 1–16.
4. A musical key writes its note directly to the selected step.
5. `K1` note, `K2` REST/NOTE/TIE, `K3` flag combination, `K4` probability.
6. `EDIT` toggles Accent; `REC` toggles Slide.

Machine/page changes are panic-guarded against stuck live notes.

## Physical drums

1. `ARP` opens the drum programmer immediately.
2. `PRESET` selects step 1–16.
3. `K1` selects lane.
4. `K2` cycles OFF/HIT/ACCENT.
5. A mapped musical key directly selects/programs/auditions its drum lane.
6. 909 keeps real flam state.

Physical usability still needs device approval; software implementation alone is not sufficient.

---

# Display contract

## Virtual

- [x] real 16-step ACID/drum state;
- [x] selected step distinct from playback step;
- [x] direct screen editing;
- [x] 240×240 source + 480×480 2× view.

## Physical

- [x] no injected legacy Felucca step graph;
- [x] real 16-step state instead of fake selected/playhead-only strip;
- [x] ACID NOTE = amber, ACCENT NOTE = red, TIE = cyan, REST = off, SLIDE = cyan marker;
- [x] drum HIT = amber, ACCENT = red, OFF = off, 909 FLAM = cyan marker;
- [x] selected step and running playhead are independent;
- [x] persistent help footer removed;
- [x] same-page changes do not begin with a full LCD clear;
- [ ] verify no residual renderer bleed-through on real LCD;
- [ ] verify no distracting redraw artifacts remain.

---

# Startup / identity screen

- [x] original **THE ReFM B1RTH** identity;
- [x] **RB-240** display designation;
- [x] ACID A / ACID B / 808 / 909 activation sequence;
- [x] 16-step bus;
- [x] final READY state;
- [x] 240×240 logical target implementation without large bitmap assets;
- [x] no copied Roland, Propellerhead or ReBirth logos/artwork;
- [ ] verify timing/appearance on physical FM-1.

---

# Audio / live-play investigation

Software-side facts already proven:

- [x] direct MIDI into `refm_felucca_midi_packet()` produces non-zero ReFm audio;
- [x] PLAY starts the ReFm transport;
- [x] seeded Pattern A produces non-zero audio and advances steps;
- [x] target audio callback calls `refm_felucca_audio_block()` instead of Felucca `mix_block()`;
- [x] ReFm Q15 samples are handed into Felucca's existing output scaling path.

Diagnostic protocol v2 now exposes:

- [x] ReFm render-block count;
- [x] non-zero ReFm render-block count;
- [x] panel key-edge count;
- [x] consumed key-edge count;
- [x] live-note edge count;
- [x] transport-event count and current transport state.

Next hardware test must determine the silence location in this order:

1. [ ] physical key edge reaches ReFm;
2. [ ] PLAY reaches ReFm transport;
3. [ ] audio ISR continuously renders ReFm blocks;
4. [ ] rendered blocks contain non-zero samples;
5. [ ] if non-zero but output is silent, isolate DAC/codec/UAC/master-volume handoff instead of changing proven DSP.

---

# Existing engine/runtime features

## ACID A / ACID B

- [x] saw + square;
- [x] tune/pitch;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] slide + tie/rest/note sequencing;
- [x] independent dual instances;
- [x] per-ACID distortion;
- [x] fixed-point target engine + WASM HQ reference;
- [ ] compare against real TB-303 reference recordings;
- [ ] physical cutoff/resonance/env-mod calibration;
- [ ] physical square/distortion listening approval.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] independent 808/909 lanes;
- [x] synthesized baseline voices;
- [x] optional redistributable sample path;
- [x] open-hat choke;
- [x] real 909 flam state + width;
- [x] direct runtime drum audition;
- [ ] physical listening approval;
- [ ] final physical lane/edit workflow approval.

## Sequencer / song / mixer / FX

- [x] 16-step patterns + A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] quantized live manual pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] delay/reverb returns;
- [x] master Dist → Comp → PCF;
- [x] versioned project save/load;
- [ ] defer longer patterns/ratchets/automation until the basic 16-step hardware workflow is physically solid.

---

# Safety / recovery / performance

- [x] pinned Felucca 1.0.5 foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and `.fwsc` generation;
- [x] package identity/SHA/memory gates;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] physical-path diagnostic v2;
- [x] guarded one-shot beta publisher;
- [x] beta.3 remains immutable.

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

- [x] `main` builds temporary host/WASM/FM-1 artifacts;
- [x] one-shot verified publisher exists;
- [x] publication is skipped without an explicit release request;
- [x] `v0.1.0-beta.3` is immutable;
- [ ] require exact-green current-head host + WASM + real FM-1 package result;
- [ ] physically validate startup, clean LCD, direct ACID/drum sequencing and audio path;
- [ ] only then create another hardware beta after explicit user request.

---

# Immediate next tasks

1. Finish the exact-head Build-all target/sample-link gate for the current direct-control + exact-pattern-readback head.
2. Flash only after an explicit new hardware-test release is requested.
3. Verify startup animation and that the old horizontal pattern strip is gone.
4. Verify footer is gone and same-page parameter changes no longer visibly blank the entire LCD.
5. Program an ACID pattern using SEQ → PRESET step → musical key / K1-K4 / EDIT / REC.
6. Program 808 and 909 patterns using ARP → PRESET step → K1/K2 / mapped keys.
7. Confirm the physical LCD shows the exact edited 16-step pattern state.
8. Capture diagnostic v2 after PLAY and live-key tests to localize any remaining silence.
9. Re-run the full physical validation checklist before considering the next beta.

---

# Definition of done for v1.0

- [x] two independent ACID engines;
- [x] 808 + 909 simultaneously available;
- [x] 16-step sequencing, patterns/song baseline;
- [x] accent/slide/probability;
- [x] mixer + essential FX;
- [x] browser installer/editor/Virtual FM-1/WASM workflow;
- [x] original startup identity;
- [x] direct software-side physical ACID/drum editing workflow;
- [x] exact physical/Virtual 16-step pattern readback;
- [ ] physical ACID sequencing is fast and predictable;
- [ ] physical 808/909 sequencing is fast and predictable;
- [ ] live keyboard play and PLAY transport are audibly proven;
- [ ] physical UI has no bleed-through or distracting refresh artifacts;
- [ ] CPU/stack headroom proven on device;
- [ ] USB MIDI/TRS MIDI/USB audio validated;
- [ ] persistence and recovery validated physically;
- [ ] no known flash/RAM/stack/CPU budget violations.
