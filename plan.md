# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

- **ACID A / ACID B** — two independent 303-inspired mono bass synths
- **808 / 909** — two distinct drum-machine workflows
- A–H patterns + song/chain sequencing
- four-channel mixer + performance FX
- USB/TRS MIDI + USB audio
- ReBirth-inspired but original 240×240 physical UI
- project persistence
- browser installer/editor + Virtual FM-1/WASM reference

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — next-beta interaction work on `main`, 2026-10-13

`v0.1.0-beta.2` is published from exact verified commit `2b4c537cc1a1f8c42c43ee953e14e27d7740837b`. **Do not overwrite or repoint beta.2.** Current `main` is an unreleased next-beta candidate until the new interaction/UI workflow is physically validated.

## Proven baseline

Checkpoint `6a784ce1e8345baee635ae4aabd4029806b3f903` is a fully green interaction baseline:

- [x] normal CI / host tests;
- [x] Build-all host tests;
- [x] WASM engine/build;
- [x] real FM-1 target compile/link/package job;
- [x] memory-budget and manifest gates;
- [x] sample-link experiment build/delta.

Later commits extend the interaction model and must obtain their own exact-head green host/WASM/FM-1 result before becoming a release candidate.

## Completed next-beta interaction work

- [x] approved RB-240 visual layout shared between physical and virtual 240×240 renderers;
- [x] shared `ui240_performance.h` behavior for hardware and WASM;
- [x] physical key events carry both key index and MIDI note;
- [x] ACID selected-step note entry and live current-step capture;
- [x] immediate drum audition plus selected/current-step programming;
- [x] distinct 808 and 909 keyboard maps;
- [x] 909 LOW/HIGH entry, real sequencer flam toggle and flam-width controls;
- [x] direct mixer mute/solo and machine-selection layer;
- [x] separate PCF / Delay / Dist / Comp selection and performance FX throws;
- [x] ACID EDIT shortcuts for Accent / Slide / Tie / Rest / probability;
- [x] SEQ / ARP / GLO / FX / SAVE / EDIT / REC / SCL temporary modifier layers;
- [x] **tap-vs-hold arbitration**: primary button action is deferred until release and suppressed when a key/encoder consumes the hold;
- [x] identical tap/hold contract in physical overlay and built Virtual FM-1;
- [x] FOCUS navigation for ACID / STEP / DRUM / MIXER / PCF / DELAY / DIST / COMP;
- [x] PRESET / ALGORITHM focus navigation and HOME panic-safe escape;
- [x] eight-position cyan FOCUS location map mirrored on hardware and Virtual FM-1;
- [x] direct Pattern A–H keyboard selection;
- [x] **quantized manual pattern changes**: immediate while stopped, queued to the next 16-step boundary while playing;
- [x] pending manual pattern requests resolve on STOP;
- [x] regression coverage for quantized pattern requests;
- [x] browser **VIRTUAL FM-1 HELP / README** directly below the virtual hardware;
- [x] help documents controls, tap/hold layers, FOCUS, 808/909 maps and PC keyboard shortcuts;
- [x] WASM build assertions verify the help module, performance ABI and FOCUS renderer patch;
- [x] beta publication guard verified: no release/channel write occurs without an explicit one-shot request.

---

# Interaction contract

## Tap vs hold

Modifier-capable buttons have one deterministic contract on hardware and Virtual FM-1:

1. press button → activate its temporary modifier layer and defer the normal button action;
2. use a keyboard key or encoder → mark the hold consumed;
3. release button → clear the modifier;
4. if consumed, do **not** fire the normal button action;
5. if not consumed, fire the normal button action on release.

This avoids accidental page changes during performance gestures without inventing different hardware/browser timing rules.

## Held layers

| Held button | Temporary behavior |
| --- | --- |
| `SEQ` | ACID note entry into selected step |
| `ARP/DRUM` | 808/909 drum-entry bank |
| `GLO/MIXER` | mute / solo / machine select |
| `FX` | PCF / Delay / Dist / Comp select and FX throws |
| `SAVE` | Pattern A–H direct select |
| `EDIT` | contextual edit shortcuts / drum high-accent entry |
| `REC` | live capture into running step |
| `SCL/SEL` | FOCUS navigation |

## Pattern switching

- stopped transport: Pattern A–H request applies immediately;
- running transport: request is stored in `pending_pattern`;
- next 16-step boundary stores the active pattern and switches to the requested pattern;
- manual pending request takes precedence over song advancement on that boundary;
- STOP resolves a pending request immediately;
- `pending_pattern` is transient runtime state and is not serialized into projects.

## FOCUS / quick zoom

FOCUS is logical-panel zoom, not bitmap scaling.

- [x] first eight physical keys select ACID, STEP, DRUM, MIXER, PCF, DELAY, DIST, COMP;
- [x] PRESET / ALGORITHM move through logical focus regions;
- [x] K1–K4 retain the focused page's native editing contract;
- [x] hardware and WASM expose focus active + region state;
- [x] HOME exits focus and performs the normal panic-safe escape;
- [x] physical and virtual RB-240 headers show the same eight-position cyan region map.

---

# ACID A / ACID B

- [x] saw + square;
- [x] tune/pitch;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] slide + tie/rest/note sequencing;
- [x] independent dual instances;
- [x] per-ACID distortion;
- [x] source-note slide semantics;
- [x] fixed-point target engine + WASM HQ reference;
- [x] keyboard selected-step entry;
- [x] REC live current-step capture;
- [x] EDIT shortcuts;
- [ ] compare against real TB-303 reference recordings;
- [ ] physical FM-1 cutoff/resonance/env-mod calibration;
- [ ] physical square/distortion listening approval.

Do not reintroduce non-stock LFO/ADSR UI into the ACID workflow.

---

# 808 / 909

- [x] 11 logical voices per machine;
- [x] synthesized baseline voices;
- [x] independent 808/909 lanes;
- [x] open-hat choke behavior;
- [x] optional sample path + auditable sample tooling;
- [x] conservative FM-1 sample link-size experiment;
- [x] real 909 flam state + width;
- [x] 909 LOW/HIGH keyboard entry;
- [x] 909 keyboard flam controls;
- [x] distinct 808 percussion ordering;
- [x] immediate drum audition;
- [x] REC live current-step drum capture;
- [ ] physical listening approval;
- [ ] confirm final physical key order/legends on hardware;
- [ ] choose final target sample subset only after physical CPU/XIP measurements.

---

# Sequencer / song / mixer / FX

- [x] 16-step patterns and A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] song mode baseline;
- [x] quantized live manual pattern switching;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] delay/reverb return processing;
- [x] master Dist → Comp → PCF signal path;
- [x] separate RB-240 PCF / Delay / Dist / Comp sections;
- [x] keyboard FX selection/throws and mixer mute/solo layer;
- [x] versioned project save/load;
- [ ] additional PCF/Dist/Comp parameters only after target profiling;
- [ ] 32/48/64-step UX after beta stability;
- [ ] ratchet beyond current 909 flam workflow;
- [ ] parameter locks/automation only after target budget validation.

The UI must not expose decorative parameters as working DSP controls.

---

# Physical RB-240 UI

Software-side implemented:

- [x] HOME, ACID, STEP, DRUM, MIXER, FX, SONG and PROJECT pages;
- [x] distinct 808 and 909 DRUM views;
- [x] original metallic/LED ReBirth-inspired design without copied artwork;
- [x] physical encoders/actions route to ReFm state;
- [x] deterministic keyboard-performance overlay on pinned Felucca;
- [x] tap/hold arbitration;
- [x] FOCUS region indicator.

Physical validation still required:

- [ ] all pages readable on the real 240×240 LCD;
- [ ] K1–K4/PRESET/ALGO/SELECT match displayed labels;
- [ ] no stuck notes after live ACID press/release and page/machine changes;
- [ ] program a complete bassline from STEP + keyboard;
- [ ] program 808 beat from keyboard;
- [ ] program 909 LOW/HIGH beat and flam;
- [ ] request Pattern A–H changes while playback runs and confirm boundary switching;
- [ ] test every mute/solo and FX hold gesture;
- [ ] test FOCUS key/encoder gestures without unwanted SONG/page actions;
- [ ] confirm project save/load and power cycle;
- [ ] align panel LEDs after the interaction contract is physically approved.

---

# Browser / Virtual FM-1 parity

- [x] INSTALLER / LIVE EDITOR / VIRTUAL FM-1 / VALIDATION combined app;
- [x] physical chassis/key ordering represented in virtual unit;
- [x] RB-240 semantic display parity;
- [x] shared physical key index + MIDI note + edge behavior;
- [x] shared modifier IDs and FOCUS state;
- [x] shared tap/hold consumed-gesture contract;
- [x] quantized pattern switching uses shared groovebox runtime;
- [x] eight-position FOCUS indicator mirrored in virtual renderer;
- [x] expandable Virtual FM-1 help/readme under the instrument;
- [x] WASM build asserts help/performance/focus integration;
- [ ] add higher-level browser interaction tests for press → key/encoder → release sequences;
- [ ] final pixel-font/LED polish after real LCD comparison.

---

# Safety / recovery / performance gates

Already implemented:

- [x] pinned Felucca 1.0.5 foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and real `.fwsc` generation;
- [x] package identity/SHA and memory gates;
- [x] loader-mediated installer and official-V15 return-to-stock identity;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] browser diagnostic capture and physical validation report;
- [x] beta.2 validation runbook.

Still required on physical hardware for the final next-beta candidate:

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
- [x] `v0.1.0-beta.2` is pinned to its existing exact commit/package;
- [x] publication guard has been observed skipping release/channel mutation without a request;
- [ ] require one exact-green **current** next-beta candidate commit with host + WASM + real FM-1 package success;
- [ ] exercise the complete tap/hold/focus/pattern workflow in Virtual FM-1;
- [ ] exercise the same workflow on physical FM-1 and capture evidence;
- [ ] only then prepare a new beta tag/channel.

---

# Immediate next tasks

1. Require the latest interaction/documentation head to pass normal CI, WASM and real FM-1 target package jobs from one exact commit.
2. Add browser-level gesture regression tests for tap, consumed hold, key release and encoder-consumed hold.
3. Exercise quantized Pattern A–H switching in Virtual FM-1 during playback.
4. Exercise every modifier layer and confirm release restores normal keyboard behavior.
5. Run the physical interaction checklist without changing beta.2.
6. Capture CPU/DMA/stack diagnostics with the final interaction layer active.
7. Fix any hardware interaction issue in the shared contract/overlay, not with divergent virtual-only mappings.
8. Prepare the next beta only after exact-green software evidence plus physical approval.

---

# Definition of done for v1.0

- [x] two independent ACID engines;
- [x] 808 + 909 simultaneously available;
- [x] patterns/song baseline and quantized live pattern requests;
- [x] accent/slide/probability;
- [x] mixer + essential FX;
- [x] combined browser installer/editor/Virtual FM-1/WASM workflow;
- [x] dedicated physical ReFm/RB-240 UI software-side;
- [x] shared keyboard-first hardware/virtual interaction layer;
- [x] distinct 808/909 workflows and real 909 flam controls;
- [x] PCF / Delay / Dist / Comp separation;
- [x] tap/hold modifier arbitration and quick FOCUS navigation;
- [x] browser help/readme for the virtual hardware;
- [x] runtime diagnostics and release provenance foundation;
- [ ] final physical CPU/stack headroom proven;
- [ ] physical UI, keyboard layers and FOCUS validated;
- [ ] USB MIDI/TRS MIDI/USB audio validated;
- [ ] save/load validated physically;
- [ ] recovery/return-to-official proven;
- [ ] all final samples have auditable licenses;
- [ ] no known flash/RAM/stack/CPU budget violations.
