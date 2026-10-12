# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 using the pinned Felucca hardware foundation.

Instrument concept:

- **ACID A** — 303-inspired mono bass synth
- **ACID B** — second independent acid synth
- **808** — synthesized 808-inspired drum machine
- **909** — synthesized/sample-hybrid 909-inspired drum machine
- A–H pattern sequencing + song/chain mode
- four-channel mixer
- distortion, delay, reverb, compressor/performance filtering
- USB/TRS MIDI + USB audio through Felucca
- project persistence
- WASM reference/emulator
- browser installer + direct hardware editor

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — 2026-10-12

## Implemented

- [x] pinned Felucca 1.0.5 foundation at `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] GPL/provenance documentation (`UPSTREAM.md`, third-party DSP/sample docs);
- [x] reproducible Linux/JieLi AC79 GitHub target build;
- [x] real `.fwsc` target package generation;
- [x] SHA-256 release artifacts + release manifest;
- [x] static XIP/RAM/pool budget gate;
- [x] two independent ACID engines;
- [x] 808 + 909, 11 lanes each;
- [x] A–H pattern bank;
- [x] song sequencer;
- [x] MIDI routing for ACID A/B, 808, 909;
- [x] start/stop/continue and external clock path;
- [x] four-channel mixer;
- [x] distortion, delay and shared reverb;
- [x] project save/load using Felucca CRC-checked A/B storage;
- [x] WASM hardware-rack editor/emulator;
- [x] TB-303 step-write workflow in browser UI;
- [x] browser AudioWorklet playback path;
- [x] host DSP/integration/stress tests;
- [x] deterministic audio render fingerprint infrastructure;
- [x] target CPU-report tooling for physical `felucca_dbg` captures;
- [x] guarded experimental command-line installer path;
- [x] hardware-test GitHub Release workflow for `.fwsc` publishing;
- [x] first integrated browser **Installer + Live Editor** under `web/app`;
- [x] Web MIDI package/device identity preflight;
- [x] browser `.fwsc` SHA-256 display;
- [x] browser OTA install/resume using the proven Felucca loader-mediated protocol;
- [x] browser refuses ambiguous multiple FM-1 MIDI devices;
- [x] direct live editor CC control for cutoff/resonance/env-mod/decay/accent/distortion/wave, level/pan/delay/reverb sends, mute/solo;
- [x] GitHub Pages deployment workflow for the installer/editor.

## Important architecture decision

The browser/WASM ACID engine is the high-quality sound reference and may use more CPU/memory than the FM-1. The embedded FM-1 engine stays fixed-point and target-budget constrained. Sound changes must be evaluated against js303/Open303/reference recordings instead of adding unverified coloration.

---

# Safety gate before first physical flash

The project uses Felucca's update loader and does **not** intentionally raw-write arbitrary flash offsets during normal installation.

Required before/for the first physical test:

- [x] clean target build;
- [x] host tests;
- [x] WASM build;
- [x] package SHA-256;
- [x] package identity validation;
- [x] XIP/RAM/pool static gate;
- [x] recovery documentation;
- [x] official V15 SHA pinned for return-to-stock verification;
- [x] FM-1-transporter documented as last-resort recovery;
- [x] loader-mediated browser installer;
- [ ] verify the exact hardware-test tag/release build is green before flashing;
- [ ] prepare known-good official V15 package locally;
- [ ] confirm OCT-/boot/update recovery path on the test device;
- [ ] have FM-1-transporter/RP2040 recovery hardware ready;
- [ ] perform first controlled install;
- [ ] prove boot/display/audio/buttons after install;
- [ ] prove USB MIDI + TRS MIDI;
- [ ] prove USB audio;
- [ ] prove save/load on device;
- [ ] prove return to official/Felucca firmware after the first experiment.

No custom firmware has a zero-brick guarantee.

---

# M0 — Foundation

- [x] pin Felucca upstream;
- [x] preserve GPL notices and upstream provenance;
- [x] build with JieLi AC79 toolchain;
- [x] generate `.fwsc`;
- [x] CI host/WASM/target builds;
- [x] target memory report.

---

# M1/M2 — ACID A + ACID B

Implemented:

- [x] saw + square;
- [x] tune/pitch;
- [x] cutoff;
- [x] resonance;
- [x] Env Mod;
- [x] decay;
- [x] accent;
- [x] slide;
- [x] tie/rest/note sequencing;
- [x] oscillator phase continuity;
- [x] two independent instances;
- [x] per-track distortion;
- [x] source-note slide semantics;
- [x] high-register oscillator regression tests;
- [x] WASM HQ reference path;
- [x] js303-informed square/control/VCA baseline.

Open sound work:

- [ ] controlled A/B against real TB-303 reference recordings at matched knob positions;
- [ ] calibrate FM-1 fixed-point cutoff/resonance/env-mod position mapping against the approved WASM reference;
- [ ] validate square character on physical FM-1 DAC/headphones;
- [ ] validate diode-style distortion level/knee on hardware;
- [ ] profile whether any HQ improvements can safely move from WASM to FM-1.

Do not reintroduce non-stock LFO/ADSR UI into the ACID workflow.

---

# M3/M4 — 808 + 909

- [x] 11 logical drum voices per machine;
- [x] synthesized baseline voices;
- [x] independent 808/909 pattern lanes;
- [x] open-hat choke behavior;
- [x] optional sample voice path;
- [x] CC0/Public Domain sample provenance tooling;
- [x] WASM full audition sample set;
- [x] conservative FM-1 sample candidate budget experiment;
- [ ] listening approval of each synthesized drum voice;
- [ ] decide final firmware sample subset after physical CPU/XIP test;
- [ ] improve individual voice parameters/editor controls where audible benefit is proven.

Target samples remain opt-in until the hardware budget and listening tests pass.

---

# M5 — Sequencer, song, mixer and FX

Implemented:

- [x] 16-step native pattern;
- [x] A–H banks;
- [x] pattern quantized switching;
- [x] song mode;
- [x] probability;
- [x] accent/slide/tie;
- [x] MIDI clock integration;
- [x] mixer level/pan;
- [x] mute/solo state;
- [x] delay sends;
- [x] reverb sends;
- [x] master distortion;
- [x] delay;
- [x] lightweight stereo reverb;
- [x] compressor/performance filtering baseline;
- [x] save/load project state.

Open:

- [ ] 32/48/64-step extended pattern UX;
- [ ] richer drum ratchet/flam editing;
- [ ] parameter locks/automation;
- [ ] longer delay/reverb on FM-1 using a memory-efficient implementation if hardware profiling permits;
- [ ] performance shortcuts such as stutter/filter sweep only after core hardware beta is stable.

---

# M6 — FM-1 physical UI

Target: a page-based 240×240 hardware groovebox, not the desktop rack squeezed onto the LCD.

Current state:

- [x] ReFm target bridge exposes BPM/pattern/step;
- [x] ACID filter graph model;
- [x] compact 16-step light strip overlay;
- [x] Felucca renderer remains the hardware foundation;
- [x] UI mockup/design direction defined for ACID, sequencer, drums, mixer, chain/song and menu.

Open and high priority:

- [ ] remove stale physical ADSR/LFO bindings from ReFm ACID pages;
- [ ] implement dedicated **Home** page: BPM, play, pattern, A/B/808/909 status;
- [ ] implement **ACID main** page: wave, cutoff, resonance, Env Mod, decay;
- [ ] implement **ACID secondary** page: tune, accent, distortion, level;
- [ ] implement **ACID step-write** page with NOTE/REST/TIE, pitch, accent and slide;
- [ ] implement **808/909** lane selection + 16-step programming page;
- [ ] implement **Mixer** page with level/mute/solo/delay/reverb sends;
- [ ] implement **FX** page for distortion/delay/reverb/master;
- [ ] implement **Pattern/Chain/Song** pages;
- [ ] implement physical project save/load controls;
- [ ] add host-render snapshots/tests for all ReFm-specific 240×240 pages.

This is the largest remaining software item before the UI shown in the mockups exists on the physical display.

---

# M6.1 — Browser installer + hardware editor

Implemented first usable version:

- [x] `web/app/index.html` integrated installer/editor;
- [x] local `.fwsc` selection/drop;
- [x] load latest `.fwsc` from GitHub Releases;
- [x] SHA-256 + package identity display;
- [x] Web MIDI + SysEx connection;
- [x] running-device identity handshake;
- [x] model mismatch rejection;
- [x] multiple-device ambiguity rejection;
- [x] explicit `FLASH THE FM-1` safety unlock;
- [x] OTA loader handoff;
- [x] interrupted-loader resume;
- [x] post-install identity verification;
- [x] live ACID controls;
- [x] live mixer sends/level/pan/mute/solo;
- [x] note audition;
- [x] drum audition pads;
- [x] A–H pattern program change;
- [x] GitHub Pages action.

Next editor work:

- [ ] device-to-editor state readback instead of transmit-only CC controls;
- [ ] transport controls;
- [ ] actual 16-step hardware pattern editing/backup via a compact SysEx protocol;
- [ ] full 808/909 lane parameter editing;
- [ ] shared master FX editing;
- [ ] project backup/restore;
- [ ] release selector with manifest/SHA cross-check against `manifest.json`/`SHA256SUMS`;
- [ ] PWA/offline shell after installer behavior is hardware-proven.

---

# M6.2 — Physical performance gates

Still blocking any claim of hardware-beta completion:

- [ ] capture physical `felucca_dbg` under the four-machine worst-case stress pattern;
- [ ] `late == 0`;
- [ ] sustained audio CPU <= 75%;
- [ ] peak audio render <= 82% of the DMA-half deadline;
- [ ] runtime user-stack high-water measurement;
- [ ] runtime system/ISR-stack high-water measurement;
- [ ] verify guard bands remain intact;
- [ ] several-minute UI + MIDI + transport stress test;
- [ ] repeated update/rollback test.

`tests/bench_groovebox.c` is a regression benchmark only. Its x86 realtime factor is not an FM-1 CPU percentage.

---

# M7 — Persistence and migration

- [x] versioned project payload;
- [x] CRC/checksum path;
- [x] Felucca A/B atomic commit storage;
- [x] A–H pattern persistence;
- [x] song/mixer/FX persistence baseline;
- [ ] explicit migration tests once project format changes after first hardware beta;
- [ ] corrupted-on-device project recovery test on physical hardware.

---

# M8 — MIDI / editor protocol

- [x] CH1 ACID A;
- [x] CH2 ACID B;
- [x] CH3 808;
- [x] CH4 909;
- [x] CH10 selected drum target;
- [x] notes;
- [x] program/pattern change;
- [x] major ACID CC controls;
- [x] mixer level/pan/delay/reverb/mute/solo CC controls;
- [x] panic/all-notes-off path;
- [ ] compact ReFm SysEx state/editor protocol;
- [ ] pattern dump/restore;
- [ ] project dump/restore;
- [ ] transport controls in browser editor;
- [ ] state subscription/readback.

---

# M9 — Browser/WASM reference

- [x] shared sequencer/groovebox core;
- [x] WebAudio output;
- [x] AudioWorklet consumer;
- [x] ReBirth-inspired original rack UI;
- [x] dual ACID panels;
- [x] 808/909 grids;
- [x] sample audition;
- [x] FX board;
- [x] pattern JSON import/export;
- [x] deterministic render tests;
- [ ] move WASM rendering fully off the main thread;
- [ ] merge the transitional timing overlay cleanly into the main rack code;
- [ ] add objective js303/Open303 comparison renders to CI;
- [ ] eventually consolidate the WASM rack and direct hardware editor into one polished application.

---

# Release / GitHub

- [x] normal main builds publish temporary Actions artifacts;
- [x] hardware-test/v* tag builds can publish GitHub Releases;
- [x] release contains `.fwsc`, app binary, OTA binary, manifest, checksums and memory report;
- [x] hardware-test/alpha/beta/rc tags are prereleases;
- [x] browser app can fetch a released `.fwsc`;
- [ ] publish first `hardware-test-*` tag only after the exact tagged CI run is green;
- [ ] optionally register a later proven release with the public FM-1 Firmware Hub.

---

# Immediate next tasks

1. **Finish/validate the integrated installer/editor build** and deploy it through GitHub Pages.
2. **Replace the stale Felucca ACID ADSR/LFO physical pages** with real ReFm ACID main + step-write pages.
3. Add the remaining **drum, mixer, FX, song and project pages** for the 240×240 display.
4. Add a **compact SysEx editor protocol** for pattern/state readback so the web editor can become bidirectional.
5. Add a **runtime stack watermark** diagnostic and report it alongside `felucca_dbg`.
6. Run the exact tagged target build and perform the **first controlled hardware installation** with recovery resources prepared.
7. Capture physical CPU/underrun/stack data before enabling larger target FX buffers or target drum samples.
8. Continue 303 sound calibration only through controlled A/B comparisons; avoid another speculative oscillator/filter rewrite.

---

# Definition of done for v1.0

- [x] two independently playable/sequenced acid machines;
- [x] 808 + 909 run simultaneously;
- [x] pattern + song baseline;
- [x] accent/slide/probability;
- [x] mixer + essential FX baseline;
- [x] browser emulator/reference;
- [x] release hashes/provenance;
- [x] safe loader-mediated installer foundation;
- [ ] proven comfortable FM-1 CPU headroom;
- [ ] proven runtime stack headroom;
- [ ] complete physical 240×240 ReFm UI;
- [ ] bidirectional browser hardware editor;
- [ ] USB MIDI/TRS MIDI/USB audio verified on physical beta;
- [ ] save/load verified on physical beta;
- [ ] recovery/return-to-official proven on hardware;
- [ ] all final bundled samples have auditable licenses;
- [ ] no known flash/RAM/stack/CPU budget violations.
