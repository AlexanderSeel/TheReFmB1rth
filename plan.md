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

`v0.1.0-beta.2` is already published from exact verified commit `2b4c537cc1a1f8c42c43ee953e14e27d7740837b`. **Do not overwrite or repoint beta.2.** All work below is an unreleased next-beta candidate until the new interaction/UI workflow is stable and physically validated.

## Completed in the current interaction batch

- [x] keep the approved RB-240 240×240 visual layout as the common hardware/virtual design;
- [x] introduce shared `ui240_performance.h` behavior instead of separate hardware/WASM musical mappings;
- [x] physical key events now carry both FM-1 physical key index and MIDI note into the ReFm interaction layer;
- [x] Virtual FM-1 build exports the same keyboard-performance ABI and passes the same physical key index ordering;
- [x] ACID keyboard note entry writes directly into the selected sequencer step;
- [x] live-record layer writes ACID notes into the current running step;
- [x] drum keyboard entry auditions the voice immediately and writes the selected/current step;
- [x] 808 and 909 use deliberately different keyboard mappings;
- [x] 909 has LOW/HIGH keyboard banks plus real sequencer flam toggle and flam-width actions;
- [x] 808 keeps rim/clap/percussion voices earlier in the physical keyboard mapping;
- [x] Pattern A–H can be selected directly from the keyboard through a held modifier;
- [x] mixer modifier exposes direct track mute/solo and machine selection;
- [x] FX modifier exposes direct PCF / Delay / Dist / Comp selection plus safe performance throws;
- [x] EDIT modifier exposes ACID Accent / Slide / Tie / Rest / probability shortcuts;
- [x] button-hold modifier model added for SEQ / ARP / GLO / FX / SAVE / EDIT / REC / SCL;
- [x] FOCUS navigation mode added with direct keyboard selectors for ACID / STEP / DRUM / MIXER / PCF / Delay / Dist / Comp;
- [x] PRESET/ALGORITHM navigate focus regions while FOCUS is active;
- [x] HOME remains the panic-safe escape and exits FOCUS;
- [x] hardware ALT behavior now matches virtual 909 flam behavior;
- [x] host tests added for distinct drum mapping, high/low entry, flam, direct patterns, mute/solo, FX selection, focus navigation and ACID step note entry;
- [x] WASM build now includes the shared performance layer and patches the built Virtual FM-1 panel to use it;
- [x] deterministic Felucca target overlay now adds the keyboard-performance mapping before the RB-240 visual patches.

## Current validation status

At checkpoint `6a784ce1e8345baee635ae4aabd4029806b3f903`:

- [x] normal CI/host tests green;
- [x] Build-all host tests green;
- [x] WASM engine/build green;
- [ ] real FM-1 target package job still needs a completed green result for the final interaction checkpoint;
- [ ] interaction workflow still needs physical rehearsal before preparing a new beta.

Documentation-only commits after that checkpoint do not constitute a new release candidate by themselves. The final candidate must again pass host, WASM and real FM-1 target package builds from one exact head commit.

---

# Interaction contract

## Normal keyboard behavior

- On ACID A/B, unmodified keys remain live note input.
- Note-off must always reach the selected synth unless the corresponding note-on was consumed by an editor/performance layer.
- Machine/page changes retain panic/stuck-note protection.

## Held layers

| Held button | Temporary keyboard behavior |
| --- | --- |
| `SEQ` | ACID note entry into selected step |
| `ARP/DRUM` | drum-entry bank |
| `GLO/MIXER` | mute / solo / machine select |
| `FX` | PCF / Delay / Dist / Comp select and FX throws |
| `SAVE` | Pattern A–H direct select |
| `EDIT` | contextual edit shortcuts / drum high-accent entry |
| `REC` | live capture into running step |
| `SCL/SEL` | enter FOCUS navigation |

Short-press primary actions remain available. If the physical hold UX proves too eager during testing, add explicit hold-time/consumed-button arbitration before the next beta rather than changing the musical mappings independently between hardware and virtual.

## FOCUS / quick zoom

FOCUS is a logical-panel zoom, not bitmap scaling.

- [x] first eight physical keys select ACID, STEP, DRUM, MIXER, PCF, DELAY, DIST, COMP;
- [x] PRESET/ALGORITHM move through logical focus regions;
- [x] K1–K4 keep the focused page's native editing contract;
- [x] focus state is exposed through both target and WASM panel APIs;
- [x] HOME exits focus and performs the normal panic-safe escape;
- [ ] add a small explicit focus/location badge to both physical and virtual RB-240 renderers if hardware legibility requires it;
- [ ] validate that entry/exit can be performed reliably during playback without accidental SONG/PAGE actions.

---

# M1/M2 — ACID A + ACID B

- [x] saw + square;
- [x] tune/pitch;
- [x] cutoff/resonance/Env Mod/decay/accent;
- [x] slide + tie/rest/note sequencing;
- [x] two independent instances;
- [x] per-ACID distortion;
- [x] source-note slide semantics;
- [x] fixed-point target engine + WASM HQ reference;
- [x] deterministic audio regression tests;
- [x] keyboard-first selected-step note entry;
- [x] live current-step note capture layer;
- [x] keyboard edit shortcuts for accent/slide/tie/rest/probability;
- [ ] match A/B against real TB-303 reference recordings;
- [ ] physical FM-1 cutoff/resonance/env-mod calibration;
- [ ] physical square/distortion listening approval.

Do not reintroduce non-stock LFO/ADSR UI into the ACID workflow.

---

# M3/M4 — 808 + 909

- [x] 11 logical voices per machine;
- [x] synthesized baseline voices;
- [x] independent 808/909 lanes;
- [x] open-hat choke behavior;
- [x] optional sample voice path + auditable sample tooling;
- [x] conservative FM-1 sample link-size experiment;
- [x] real 909 flam sequencer state and width;
- [x] 909 LOW/HIGH keyboard entry;
- [x] 909 keyboard flam toggle/width performance actions;
- [x] distinct 808 physical-key percussion ordering;
- [x] immediate drum audition on keyboard entry;
- [x] live current-step drum capture through REC layer;
- [ ] physical listening approval of synthesized voices;
- [ ] confirm the final 808/909 physical key legends/muscle-memory order on hardware;
- [ ] choose final firmware sample subset only after physical CPU/XIP measurements.

---

# M5 — Sequencer / song / mixer / FX

- [x] native 16-step patterns;
- [x] A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] song mode baseline;
- [x] mixer level/pan/mute/solo;
- [x] per-track delay/reverb sends;
- [x] master distortion, compression and PCF/low-pass processing;
- [x] delay and reverb send/return processing;
- [x] separate RB-240 **PCF / Delay / Dist / Comp** sections;
- [x] keyboard FX section selection and performance throws;
- [x] direct mixer mute/solo keyboard layer;
- [x] versioned project save/load;
- [ ] consider additional PCF/Dist/Comp parameters only after target profiling; do not add decorative non-audible controls;
- [ ] 32/48/64-step UX after beta stability;
- [ ] ratchet beyond the implemented 909 flam workflow;
- [ ] parameter locks/automation;
- [ ] larger target FX only if physical profiling permits.

### Current mixer/FX signal contract

1. four machine inputs → per-track level/pan/mute/solo;
2. per-track delay/reverb sends feed their return paths;
3. dry + delay + reverb returns are summed;
4. summed signal → Dist → Comp → PCF/master low-pass → output.

The UI must describe this truthfully. Do not show unsupported PCF resonance, compressor attack/release, distortion tone, or delay-pan controls as if they were active DSP parameters.

---

# M6 — FM-1 physical RB-240 UI

Implemented software-side:

- [x] HOME page with machine/pattern/BPM/transport status;
- [x] ACID page with Cutoff / Resonance / Env Mod / Decay + waveform;
- [x] STEP page with 16-step selection, pitch, NOTE/REST/TIE, Accent/Slide, probability;
- [x] distinct 808 and 909 DRUM views;
- [x] MIXER page with Level / Pan / Delay / Reverb + Mute/Solo;
- [x] separate PCF / Delay / Dist / Comp FX panels;
- [x] SONG and PROJECT beta pages;
- [x] original metallic/LED ReBirth-inspired visual language without copied artwork;
- [x] hardware and virtual layout use the same 240×240 semantic design;
- [x] physical encoders/actions route into ReFm rather than Felucca's old audible state;
- [x] keyboard modifier/focus state added to the target bridge;
- [x] hardware keyboard-performance overlay is deterministic and applied to pinned Felucca during target build.

Physical validation required before next beta:

- [ ] confirm all pages render correctly on the real 240×240 LCD;
- [ ] confirm K1–K4/PRESET/ALGO/SELECT match the displayed labels;
- [ ] confirm live ACID press/release with no stuck notes;
- [ ] enter a bassline entirely from STEP + keyboard;
- [ ] program an 808 beat from the keyboard;
- [ ] program a 909 beat with LOW/HIGH and flam;
- [ ] change Patterns A–H while playback runs;
- [ ] mute/solo each of the four machines from the keyboard layer;
- [ ] perform PCF/delay/dist/comp shortcuts during playback;
- [ ] enter FOCUS, move between panels, edit with K1–K4 and return to notes without stopping playback;
- [ ] confirm project save/load on device;
- [ ] align physical panel LED behavior after the interaction contract is proven.

---

# M6.1 — Browser / Virtual FM-1 parity

- [x] combined INSTALLER / LIVE EDITOR / VIRTUAL FM-1 / VALIDATION app;
- [x] Virtual FM-1 follows the physical 1000×640 chassis geometry and 27-key ordering;
- [x] virtual display mirrors RB-240 page semantics and geometry;
- [x] virtual encoders/actions use the same `ui240` model;
- [x] built virtual panel now sends physical key index + note + edge into the shared keyboard interaction layer;
- [x] virtual button holds use the same modifier ids as the physical overlay;
- [x] virtual FOCUS uses the same state/page selection logic as hardware;
- [x] WASM exports modifier/focus/performance key/encoder/action accessors;
- [x] WASM build asserts those exports and the patched virtual mapping are present;
- [ ] final pixel-font/LED/focus-indicator polish after physical screen comparison;
- [ ] add higher-level browser interaction tests for modifier press→key→release sequences.

---

# Safety / recovery and performance gates

Already implemented:

- [x] pinned Felucca 1.0.5 foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and real `.fwsc` generation;
- [x] package identity/SHA validation and XIP/RAM/pool gates;
- [x] loader-mediated installer and official V15 return-to-stock identity;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] browser diagnostic capture and offline physical validation report;
- [x] beta.2 runbook at `docs/PHYSICAL_BETA2_VALIDATION.md`.

Still required on hardware:

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
- [ ] repeat update/rollback cycle several times.

No custom firmware has a zero-brick guarantee.

---

# Release / GitHub policy

- [x] main builds temporary host/WASM/FM-1 artifacts;
- [x] tagged builds can publish `.fwsc` releases;
- [x] verified one-shot beta publisher exists;
- [x] `v0.1.0-beta.2` published and its browser firmware channel pinned to its exact commit/SHA;
- [x] current interaction work leaves beta.2 unchanged;
- [ ] require one exact-green next-beta candidate commit with host + WASM + real FM-1 package success;
- [ ] exercise the new keyboard/hold/focus workflow on Virtual FM-1;
- [ ] exercise the same workflow on physical FM-1 and capture validation evidence;
- [ ] only then prepare the next beta tag/channel.

---

# Immediate next tasks

1. Finish the current exact-head Build-all run and require **Host tests + WASM engine + FM-1 target package** all green.
2. Add/verify a small visible FOCUS/location indicator only if the real 240×240 screen needs it; keep hardware and virtual identical.
3. Exercise every held layer in Virtual FM-1, especially press/hold/release ordering and return to normal live notes.
4. Verify 808 and 909 physical-key mapping is comfortable and unambiguous before freezing legends.
5. Run the physical interaction checklist above without republishing beta.2.
6. Re-run CPU/DMA/stack diagnostics with the interaction layer active.
7. Fix any physical hold-timing or accidental-primary-action issues in the shared layer/overlay, not with target-specific divergent mappings.
8. Produce a fresh exact-green commit, artifact hashes and physical evidence before preparing the next beta.

---

# Definition of done for v1.0

- [x] two independent ACID engines;
- [x] 808 + 909 simultaneously available;
- [x] patterns/song baseline;
- [x] accent/slide/probability;
- [x] mixer + essential FX;
- [x] combined browser installer/editor/virtual FM-1/WASM workflow;
- [x] dedicated physical ReFm/RB-240 UI implemented software-side;
- [x] shared keyboard-first hardware/virtual interaction layer;
- [x] distinct 808/909 keyboard workflows and real 909 flam controls;
- [x] PCF / Delay / Dist / Comp panel separation;
- [x] modifier layers and quick FOCUS navigation implemented software-side;
- [x] runtime diagnostics and validation evidence pipeline;
- [x] release hashes/provenance and loader-mediated installer foundation;
- [ ] comfortable FM-1 CPU/stack headroom proven physically with the final interaction build;
- [ ] physical UI, keyboard layers and FOCUS validated;
- [ ] USB MIDI/TRS MIDI/USB audio validated;
- [ ] save/load validated physically;
- [ ] recovery/return-to-official proven;
- [ ] all final samples have auditable licenses;
- [ ] no known flash/RAM/stack/CPU budget violations.
