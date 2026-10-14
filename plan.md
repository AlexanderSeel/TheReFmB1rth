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

# Current checkpoint — beta.4 failed physical validation, 2026-10-14

`v0.1.0-beta.4` is the latest published hardware-test prerelease, but **it failed physical usability validation and must not be treated as a stable baseline**. Do not overwrite or repoint beta.4. Current `main` is an unreleased hardware-stabilization branch state on the normal `main` branch.

The beta.4 device test established:

- [x] update/install and boot still work;
- [x] the firmware can navigate to ACID, STEP, DRUM, MIX, FX and SONG pages;
- [ ] button/encoder behavior is reliable — **FAILED on beta.4**;
- [ ] musical keyboard reaches ReFm — **FAILED on beta.4**;
- [ ] PLAY produces acceptable audio — **FAILED on beta.4: loud/annoying output**;
- [ ] physical MASTER affects ReFm output — **FAILED on beta.4**;
- [ ] knob changes redraw cleanly — **FAILED on beta.4: visible clear/rebuild/flicker**;
- [ ] all 240×240 layouts are readable — **FAILED on beta.4: crowded/clipped/overlapping content**.

These are release blockers. No beta.5 should be requested until the corrective target build is green and a deliberate hardware retest is wanted.

## Root causes confirmed after beta.4

The device failures were not independent cosmetic bugs:

1. The ReFm target replaced Felucca's normal `mix_block()` audio path.
2. Felucca's physical musical-key scan normally runs from that bypassed control/audio path, so the ReFm engine never received the physical keyboard on beta.4.
3. Felucca's physical MASTER scaling also lived in the bypassed mix path, so the MASTER control could not attenuate ReFm audio on beta.4.
4. The beta.4 LCD patch stopped one explicit full-screen clear but still rebuilt most visible regions after ordinary parameter changes, which remained visibly flickery on the real LCD.
5. Several inherited desktop-like/legacy layout passes were too dense for the physical 240×240 panel.

## Post-beta.4 hardware stabilization implemented on `main`

- [x] Musical keys are scanned directly from `fm1_in.notes` in the real audio/control ISR.
- [x] Per-key note ownership is retained so key-up sends the same note even after octave/page changes.
- [x] Keyboard note mapping is independent from Felucca's old selected-track engine state.
- [x] Physical MASTER (`song.master_q12`) is applied after the ReFm render again.
- [x] Added conservative **-6 dB target output headroom** before DAC/UAC handoff.
- [x] Output is clamped to the signed Q15 range after target gain.
- [x] Primary hardware controls use strict immediate page-first/contextual behavior.
- [x] `OCT- / OCT+` are contextual editor controls on STEP/DRUM and actual keyboard octave controls elsewhere.
- [x] Replaced the inherited physical renderer with a compact hardware-only RB-240 renderer.
- [x] Ordinary knob/step changes redraw only changed cards/cells; full 240×240 clear is reserved for page/machine transitions.
- [x] ACID and DRUM pages use exact 16-step groovebox readback.
- [x] Target renderer math is division-free/fixed-point where needed for JieLi safety.
- [x] ROM-reference diagnostics now identify the owning target symbol and nearby instructions without relaxing the safety gate.
- [x] Fixed a target safety false-positive where PAN constant folding produced arithmetic immediate `0xFFCE0064`; equivalent `×25 >> 13` scaling avoids the ROM-address bit pattern.
- [x] Exact corrective code commit `d3f0b6c0a9bfd9f1714d5aeaaef9048389df7287` passed Host, WASM and the real FM-1 app/loader/FWSC build, target memory/manifest gates and sample link-size experiment.
- [ ] Revalidate the corrective build on real FM-1 hardware.

---

# Simple physical control contract

The physical unit must remain usable without hidden modifier holds.

## ACID

1. `ENV` → ACID A.
2. `LFO` → ACID B.
3. `SEQ` → ACID step editor.
4. `PRESET` → step 1–16.
5. Musical key → write/audition the note for the selected step.
6. `K1` → note/pitch.
7. `K2` → REST / NOTE / TIE.
8. `K3` → accent/slide flags.
9. `K4` → probability.
10. `OCT- / OCT+` → pitch down/up while in STEP; real keyboard octave elsewhere.

## DRUM

1. `ARP` → drum programmer.
2. Repeated/contextual DRUM selection toggles 808/909.
3. `PRESET` → step 1–16.
4. `K1` or `OCT- / OCT+` → lane down/up.
5. `K2` → OFF / HIT / ACCENT.
6. Musical key → select/program/audition mapped drum lane.
7. 909 keeps real flam state/width.

## Other primary pages

- `HOME` → home/status.
- `GLO` → mixer.
- `FX` → effects.
- `SCL` → song/pattern.
- `SAVE` → project.
- `PLAY` → ReFm transport immediately.

Machine/page changes remain panic-guarded against stuck notes.

---

# Display contract

## Virtual

- [x] real 16-step ACID/drum state;
- [x] selected step distinct from playback step;
- [x] direct screen editing;
- [x] 240×240 source + true 480×480 2× view.

## Physical corrective renderer

- [x] no legacy Felucca graph strip;
- [x] no persistent help footer;
- [x] compact page-specific layout rather than stacked legacy layers;
- [x] exact ACID NOTE / REST / TIE / ACCENT / SLIDE readback;
- [x] exact DRUM OFF / HIT / ACCENT / 909 FLAM readback;
- [x] selected step and playhead are independent;
- [x] dirty-region/card/cell redraw for same-page changes;
- [x] full-screen clear only on page/machine transition;
- [ ] verify zero visible full-screen flash during continuous knob rotation;
- [ ] verify no clipped/overlapping labels on the real LCD;
- [ ] verify no stale pixels remain when values shrink/change.

---

# Audio / keyboard investigation

Software/target facts now proven:

- [x] direct MIDI into `refm_felucca_midi_packet()` produces non-zero ReFm audio;
- [x] PLAY starts the ReFm transport in host integration tests;
- [x] seeded Pattern A produces non-zero audio and advances steps;
- [x] ReFm owns the target render path;
- [x] physical musical-key edges are now scanned independently of Felucca `mix_block()`;
- [x] physical MASTER scaling is restored after the ReFm render;
- [x] -6 dB target headroom is applied before output handoff;
- [x] the corrective audio/input path compiles and links in the real JieLi target package.

Diagnostic protocol v2 exposes:

- [x] ReFm render-block count;
- [x] non-zero ReFm render-block count;
- [x] panel key-edge count;
- [x] consumed key-edge count;
- [x] live-note edge count;
- [x] transport-event count and transport state.

Next physical test must verify in this order:

1. [ ] STOP is silent;
2. [ ] MASTER at minimum is effectively silent and scales smoothly upward;
3. [ ] one live ACID key gives one controlled note and releases cleanly;
4. [ ] keyboard octave changes work outside editor context;
5. [ ] PLAY starts a controlled seeded pattern rather than a loud invalid signal;
6. [ ] STOP ends that signal immediately;
7. [ ] 808/909 single-hit audition is controlled;
8. [ ] no stuck key survives page/machine/octave changes;
9. [ ] capture diagnostic v2 if any of the above fails.

Do not perform initial audio tests at high headphone/amplifier volume.

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
- [ ] physical cutoff/resonance/env-mod calibration;
- [ ] physical listening approval.

## 808 / 909

- [x] 11 logical voices per machine;
- [x] independent 808/909 lanes;
- [x] synthesized baseline voices;
- [x] open-hat choke;
- [x] real 909 flam state + width;
- [x] direct runtime drum audition;
- [ ] physical listening approval;
- [ ] physical lane/edit workflow approval.

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
- [ ] defer longer patterns/ratchets/automation until the basic hardware workflow is physically solid.

---

# Safety / recovery / performance

- [x] pinned Felucca 1.0.5 foundation `3dd2b0852bc310a2c1bf00c2d443ac19202c2140`;
- [x] reproducible JieLi target build and `.fwsc` generation;
- [x] package identity/SHA/memory gates;
- [x] strict ROM-reference safety gate retained;
- [x] runtime CPU/DMA/user-stack/supervisor-stack instrumentation;
- [x] physical-path diagnostic v2;
- [x] guarded one-shot beta publisher;
- [x] beta.3 and beta.4 remain immutable.

Still required physically:

- [ ] controlled audio-output test at low external volume;
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
- [x] `v0.1.0-beta.3` and `v0.1.0-beta.4` are immutable;
- [x] corrective code checkpoint has exact-green Host + WASM + real FM-1 package gates;
- [ ] do not publish beta.5 merely because software gates are green;
- [ ] publish another hardware-test beta only after explicit user request.

---

# Immediate next tasks

1. Keep the post-beta.4 hardware correction stack as the owning final target pass.
2. Verify the physical button/encoder contract one control at a time on the next device build.
3. Verify keyboard edge → ReFm note-on/note-off independently from Felucca track state.
4. Verify MASTER minimum/maximum and -6 dB headroom before testing PLAY loudly.
5. Verify continuous knob rotation causes only local card/cell updates with no full LCD flash.
6. Verify STEP note writing and DRUM lane/hit writing are visible immediately.
7. Capture diagnostic v2 after any failed keyboard/PLAY/audio test.
8. Only after that physical evidence, iterate remaining sound/layout issues.

---

# Definition of done for v1.0

- [x] two independent ACID engines;
- [x] 808 + 909 simultaneously available;
- [x] 16-step sequencing, patterns/song baseline;
- [x] accent/slide/probability;
- [x] mixer + essential FX;
- [x] browser installer/editor/Virtual FM-1/WASM workflow;
- [x] original startup identity;
- [x] exact physical/Virtual 16-step pattern readback;
- [ ] physical ACID sequencing is fast and predictable;
- [ ] physical 808/909 sequencing is fast and predictable;
- [ ] live keyboard play and PLAY transport are audibly proven;
- [ ] MASTER controls the real ReFm output correctly;
- [ ] physical UI has no bleed-through, clipping or distracting refresh artifacts;
- [ ] CPU/stack headroom proven on device;
- [ ] USB MIDI/TRS MIDI/USB audio validated;
- [ ] persistence and recovery validated physically;
- [ ] no known flash/RAM/stack/CPU budget violations.
