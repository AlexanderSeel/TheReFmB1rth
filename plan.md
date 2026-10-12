# TheReFmB1rth — Implementation Plan

## Goal

Build a GPL-compatible, original replacement firmware for the M-VAVE FM-1 on the pinned Felucca hardware foundation:

- **ACID A / ACID B** — two independent 303-inspired mono bass synths
- **808 / 909** — two drum machines
- A–H patterns + song/chain sequencing
- four-channel mixer + distortion/delay/reverb/filtering
- USB/TRS MIDI + USB audio
- physical 240×240 groovebox UI
- project persistence
- browser installer/editor + WASM reference

No Roland/Propellerhead/ReBirth artwork, logos, proprietary source, ROMs, or non-redistributable samples are shipped.

---

# Current checkpoint — post-beta.1 corrective build, 2026-10-12

## First physical install — completed

The first custom firmware install succeeded, but the build was **not usable as an instrument**. Hardware findings:

- [x] loader-mediated installation completed successfully;
- [x] device rebooted into custom firmware;
- [x] findings captured and converted into beta acceptance criteria;
- [ ] old build had the previewed ReFm screens — **failed**; it still showed Felucca-derived UI;
- [ ] old build had responsive continuous audio — **failed**; it was laggy and often silent;
- [ ] old build had sensible physical control mapping — **failed**;
- [ ] old build had live keyboard play — **failed**;
- [ ] old build had working physical PLAY/sequencing — **failed**;
- [ ] old build had simple editing — **failed**.

Root cause confirmed: Felucca still owned the physical panel/keyboard/transport while TheReFmB1rth replaced the audible engine. The two state/control paths were therefore disconnected.

## Beta corrective architecture — implemented

- [x] physical FM-1 panel now drives the ReFm engine directly;
- [x] physical keyboard routes to selected ACID A/B;
- [x] physical PLAY starts/stops the ReFm internal sequencer;
- [x] Felucca's old audible sequencer/play/record state is held stopped while ReFm owns the beta workflow;
- [x] stale ReFm ADSR/LFO physical mappings removed;
- [x] dedicated ReFm 240×240 renderer replaces normal Felucca pages during beta operation;
- [x] renderer only repaints when compact ReFm state changes;
- [x] target-only silent ACID fast path avoids running the expensive ladder for completely idle voices;
- [x] held-note panic before switching into editor pages or changing machine;
- [x] fresh Pattern A contains an audible factory demo so PLAY is testable immediately;
- [x] host bridge test covers fresh boot → sound → PLAY → step advance → STOP → page changes;
- [x] exact JieLi target build passes Felucca ROM/register checks, memory budgets, manifest and SHA verification;
- [x] verified one-shot beta publisher workflow added;
- [x] target runtime stack-watermark instrumentation added for user + supervisor/ISR stacks;
- [x] combined physical runtime report now gates CPU, DMA lateness and minimum stack headroom;
- [x] browser can request a device-state snapshot and synchronize the live editor from the FM-1;
- [x] browser can back up and restore the complete versioned ReFm project blob, including the A–H pattern bank;
- [x] exact readback checkpoint `8e6daf2f0345e297c21c5b3270de2498f981e8ba` passed host, WASM, FM-1 target package, target-memory, manifest/SHA and sample-link experiment gates;
- [x] exact full-transfer checkpoint `993eeb40255d75bbe3b344fa9c6c400816b05820` passed CI, WASM, real JieLi FM-1 app/loader/FWSC build, target-memory budgets, release manifest/SHA verification and sample-link experiment.

The current code is a **software-gated beta candidate** until the second physical validation pass succeeds.

---

# Safety / recovery

- [x] pinned Felucca 1.0.5 foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build;
- [x] real `.fwsc` generation;
- [x] package identity + SHA-256 validation;
- [x] XIP/RAM/pool static gates;
- [x] loader-mediated browser installer + resume;
- [x] official V15 SHA pinned for return-to-stock verification;
- [x] FM-1-transporter documented as last-resort recovery;
- [x] first controlled custom firmware installation performed;
- [ ] verify OCT-/boot/update recovery path on the physical test device;
- [ ] prove return to official/Felucca firmware after beta test;
- [ ] repeat update/rollback cycle several times.

No custom firmware has a zero-brick guarantee.

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
- [ ] matched A/B against real TB-303 reference recordings;
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
- [ ] physical listening approval of synthesized voices;
- [ ] choose final firmware sample subset only after physical CPU/XIP measurements.

---

# M5 — Sequencer / song / mixer / FX

- [x] native 16-step patterns;
- [x] A–H banks;
- [x] probability + accent + slide + tie;
- [x] MIDI clock integration;
- [x] song mode baseline;
- [x] mixer level/pan/mute/solo;
- [x] delay/reverb sends;
- [x] master distortion/delay/reverb/filter/compression baseline;
- [x] versioned project save/load;
- [ ] 32/48/64-step UX after beta stability;
- [ ] ratchet/flam;
- [ ] parameter locks/automation;
- [ ] larger target FX only if physical profiling permits.

---

# M6 — FM-1 physical beta UI

Implemented software-side:

- [x] dedicated **HOME** page with machine/pattern/BPM/transport status;
- [x] dedicated **ACID** page with Cutoff / Resonance / Env Mod / Decay + wave toggle;
- [x] dedicated **STEP** page with 16-step selection + NOTE/REST/TIE + Accent/Slide + probability;
- [x] dedicated **DRUM** page with 808/909 machine, lane, step and OFF/HIT/ACCENT state;
- [x] dedicated **MIXER** page with Level / Pan / Delay / Reverb + Mute/Solo;
- [x] dedicated **FX** page with Drive / Delay feedback / Reverb mix / Filter;
- [x] **SONG** beta page focused on pattern playback/loop;
- [x] **PROJECT** page with four project slots and physical load/save controls;
- [x] LCD footer tells the user the actual physical knob/button mapping;
- [x] PRESET knob = pattern normally, selected step in step/drum editors, project slot on project page;
- [x] ALGO knob = machine selection;
- [x] SELECT knob = BPM;
- [x] ENV button = ACID A;
- [x] LFO button = ACID B (label reused as shortcut, no LFO feature);
- [x] ARP button = DRUM / 808↔909;
- [x] SEQ button = STEP;
- [x] GLO = MIXER, FX = FX, SCL = SONG, SAVE = PROJECT, HOME = HOME;
- [x] PLAY owns ReFm transport;
- [x] EDIT/REC act as contextual secondary actions;
- [x] host tests for compact UI state/editing and hardware bridge workflow.

Physical validation still required:

- [ ] confirm all dedicated pages actually render correctly on the 240×240 LCD;
- [ ] confirm K1–K4/PRESET/ALGO/SELECT physical encoders match the LCD labels;
- [ ] confirm keyboard play and release on ACID A/B with no stuck notes;
- [ ] confirm PLAY/STOP and audible factory Pattern A;
- [ ] confirm STEP and DRUM editing is comfortable on hardware;
- [ ] confirm mixer/FX editing while playing remains responsive;
- [ ] confirm project save/load on device;
- [ ] align panel LED behavior with ReFm page ownership after core beta behavior is proven.

---

# M6.1 — Browser installer + editor

- [x] local `.fwsc` selection/drop;
- [x] GitHub Release discovery;
- [x] browser-safe `firmware-channel` mirror because GitHub Release binary redirects are CORS-blocked;
- [x] mirror tag + SHA verification before package acceptance;
- [x] Web MIDI + SysEx identity handshake;
- [x] explicit `FLASH THE FM-1` safety unlock;
- [x] OTA loader handoff + resume + post-install verification;
- [x] live ACID/mixer controls;
- [x] 16-step Acid/808/909 editing protocol;
- [x] note/drum audition;
- [x] browser START / CONTINUE / STOP using the firmware MIDI realtime transport path;
- [x] device-to-editor current-state readback over a reserved channel-16 CC envelope;
- [x] readback response is deferred to the audio context so Felucca's MIDI OUT ring keeps its single-producer assumption;
- [x] browser sync updates machine/pattern/step plus ACID, drum and mixer editor state without echoing writes back to the FM-1;
- [x] full A–H pattern/project backup + restore over Felucca's existing editor SysEx framing;
- [x] project transfer reuses the normal ReFm versioned project blob instead of introducing a second serialization format;
- [x] project transfer uses 256-byte 7-bit-packed chunks, an outer CRC32, staging-generation protection and the inner ReFm project CRC/version checks;
- [x] project backup/restore is refused while ReFm transport is playing;
- [x] browser restore changes the live project only; normal physical PROJECT SAVE remains the explicit persistence step;
- [ ] consolidated polished editor/WASM application after physical beta is stable.

---

# M6.2 — Physical beta performance gates

Software-side instrumentation/tooling:

- [x] target-only user-stack watermark with conservative active-frame safety band;
- [x] target-only supervisor/ISR-stack watermark before interrupts are enabled;
- [x] stack high-water fields exposed through `felucca_dbg`;
- [x] `target_runtime_report.py` combines CPU, DMA deadline and stack-headroom gates;
- [x] host regression tests cover healthy, missing-field, low-stack and late-DMA reports;
- [x] target stack-symbol declarations/casts aligned with Felucca's linker/guard declarations.

These are the main blockers between **beta candidate** and **hardware-proven beta**:

- [ ] capture physical `felucca_dbg` while factory Pattern A plays;
- [ ] capture worst-case four-machine stress run;
- [ ] `late == 0`;
- [ ] sustained audio CPU <= 75%;
- [ ] peak audio render <= 82% of DMA-half deadline;
- [ ] physical user-stack free headroom >= 4096 bytes;
- [ ] physical supervisor/ISR-stack free headroom >= 1024 bytes;
- [ ] guard bands intact;
- [ ] several-minute UI + keyboard + MIDI + transport stress test;
- [ ] USB MIDI verified;
- [ ] TRS MIDI verified;
- [ ] USB audio verified;
- [ ] physical save/load verified;
- [ ] rollback/recovery verified.

`tests/bench_groovebox.c` is an x86 regression benchmark only; it is not an FM-1 CPU percentage.

---

# Persistence

- [x] versioned project payload;
- [x] CRC/checksum path;
- [x] Felucca A/B atomic storage;
- [x] A–H pattern persistence;
- [x] song/mixer/FX persistence baseline;
- [x] four physical beta project slots;
- [x] complete project blob can be exported/restored through the browser without bypassing the project codec;
- [ ] physical corrupted-project recovery test;
- [ ] explicit migration tests once project format changes after beta.

---

# Release / GitHub

- [x] main builds publish temporary Actions artifacts;
- [x] tag builds publish `.fwsc` GitHub Releases;
- [x] release includes firmware, app/OTA binaries, manifest, checksums, memory report and overlay provenance;
- [x] prerelease classification for hardware-test/alpha/beta/rc;
- [x] CORS-safe browser firmware channel;
- [x] one-shot `Publish verified beta` workflow only consumes a successful exact-head `Build all artifacts` result;
- [x] `v0.1.0-beta.1` published as the initial prerelease from commit `3b881e3f845a71f7fdf280b2f0b5016d84b3a254`;
- [x] explicitly treat `beta.1` as older than the later runtime-stack/readback/project-transfer corrections — do not present those fixes as part of `beta.1`;
- [ ] publish the next beta (normally `v0.1.0-beta.2`) only from a chosen exact-green corrective commit;
- [ ] second physical hardware validation;
- [ ] only later consider public FM-1 Firmware Hub registration.

---

# Immediate beta tasks

1. Preserve `993eeb40255d75bbe3b344fa9c6c400816b05820` as the first fully green target checkpoint containing runtime-stack diagnostics, device-state readback and complete browser project backup/restore.
2. Expose the physical CPU/late/stack diagnostic snapshot through the browser so the second hardware pass can collect acceptance data without manual memory reads.
3. Get that diagnostic-browser checkpoint through host + WASM + real JieLi FM-1 target + memory/package gates.
4. Publish the next prerelease only from a chosen exact-green corrective commit; do not overwrite/relabel `v0.1.0-beta.1`.
5. Install that corrective beta through the browser app.
6. Validate HOME / ACID / STEP / DRUM / MIXER / FX / SONG / PROJECT pages on the actual LCD.
7. Validate live keys, PLAY/STOP, factory Pattern A and editing without lag/stuck notes.
8. Capture factory-pattern and worst-case stress diagnostics and run `target_runtime_report.py` against them.
9. Verify USB MIDI, TRS MIDI, USB audio and physical project save/load.
10. Prove rollback/recovery and repeat the update/rollback cycle.

---

# Definition of done for v1.0

- [x] two independent ACID engines;
- [x] 808 + 909 simultaneously available;
- [x] patterns/song baseline;
- [x] accent/slide/probability;
- [x] mixer + essential FX;
- [x] browser emulator/reference + installer/editor;
- [x] release hashes/provenance;
- [x] loader-mediated installer foundation;
- [x] dedicated physical ReFm beta UI implemented software-side;
- [x] runtime user/supervisor stack instrumentation implemented software-side;
- [x] device-to-browser current-state synchronization implemented software-side;
- [x] full bidirectional browser project/pattern transfer implemented software-side;
- [ ] comfortable FM-1 CPU headroom proven physically;
- [ ] runtime stack headroom proven physically;
- [ ] physical UI/controls validated;
- [ ] USB MIDI/TRS MIDI/USB audio validated;
- [ ] save/load validated physically;
- [ ] recovery/return-to-official proven;
- [ ] all final samples have auditable licenses;
- [ ] no known flash/RAM/stack/CPU budget violations.
