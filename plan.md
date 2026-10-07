# TheReFmB1rth — Implementation Plan

## Goal

Create a GPL-compatible, ReBirth-inspired replacement firmware for the M-VAVE FM-1 built on the proven Felucca firmware foundation.

Target instrument concept:

- **ACID 1** — monophonic 303-inspired bass synth
- **ACID 2** — second independent 303-inspired bass synth
- **DRUM A** — 808-inspired drum machine
- **DRUM B** — 909-inspired drum machine
- shared pattern/song sequencer
- mixer + distortion + compressor + delay + performance filter
- USB MIDI + TRS MIDI + USB audio
- project/pattern persistence
- browser/WASM emulator before hardware flashing

The project must be an original implementation inspired by classic workflow and sound. Do not ship Roland, Propellerhead, ReBirth, TB-303, TR-808 or TR-909 artwork, logos, proprietary ROMs, copyrighted factory samples, copied source code, or other protected assets unless their license explicitly permits redistribution.

---

# Non-negotiable safety rules

The FM-1 must never be used as the first test target.

A hardware image may only be considered flashable after all of the following pass:

1. firmware builds from a clean checkout;
2. host-side DSP/sequencer/storage tests pass;
3. WASM/browser emulator starts and can render/play all machines;
4. firmware size fits the known FM-1 flash/XIP budget;
5. RAM/pool/stack use is measured and within limits;
6. CPU/render budget passes worst-case patterns;
7. package checksum is generated and verified;
8. installer performs FM-1 identity check before writing;
9. updater validates package identity/version/length before erase/write;
10. recovery instructions and official-firmware recovery path are documented before first flash;
11. the user has a known-good recovery method available before experimental flashing.

**Never bypass the Felucca update loader or write arbitrary offsets directly to flash during normal installation.**

If an install fails and the device cannot boot, recovery may require the separate FM-1-transporter hardware method. Experimental flashing therefore remains opt-in and explicitly marked unsafe until validated on real hardware.

---

# Foundation

Use **Felucca** as the hardware/firmware platform rather than starting from the factory firmware.

Reuse or adapt, under GPL-3.0-only compatibility:

- FM-1 HAL
- LCD driver and UI renderer
- key/knob/encoder/LED input layer
- audio output path
- USB MIDI
- TRS MIDI
- USB audio
- clock/sync infrastructure
- flash persistence layer
- update loader / `.fwsc` packaging
- command-line installer
- browser/WASM emulator
- host regression tests
- CPU budget tests
- project/preset backup patterns where useful

Expected target constraints inherited from Felucca:

- 44.1 kHz stereo audio
- limited XIP/code region
- roughly 96 KiB general RAM plus dedicated pool region
- fixed-point-friendly embedded DSP
- four logical tracks map naturally to 2 acid + 2 drum machines

---

# Repository structure target

```text
TheReFmB1rth/
├─ firmware/
│  ├─ hal/
│  ├─ loader/
│  └─ src/
│     ├─ app/
│     ├─ audio/
│     ├─ dsp/
│     ├─ machines/
│     │  ├─ acid303.c
│     │  ├─ acid303.h
│     │  ├─ drum808.c
│     │  ├─ drum808.h
│     │  ├─ drum909.c
│     │  └─ drum909.h
│     ├─ sequencer/
│     │  ├─ transport.c
│     │  ├─ pattern.c
│     │  ├─ song.c
│     │  ├─ swing.c
│     │  └─ automation.c
│     ├─ mixer/
│     ├─ fx/
│     ├─ storage/
│     ├─ midi/
│     └─ ui/
├─ assets/
│  ├─ samples-cc0/
│  └─ fonts/
├─ tools/
│  ├─ fetch_samples.py
│  ├─ verify_samples.py
│  ├─ fm1_install.py
│  ├─ make_package.py
│  └─ preflight.py
├─ tests/
│  ├─ dsp/
│  ├─ sequencer/
│  ├─ storage/
│  ├─ installer/
│  └─ golden/
├─ web/
│  ├─ emu/
│  └─ installer/
├─ docs/
│  ├─ FLASH_SAFETY.md
│  ├─ SAMPLE_SOURCES.md
│  ├─ ARCHITECTURE.md
│  └─ DSP_NOTES.md
├─ build.sh
├─ README.md
├─ LICENSE
└─ plan.md
```

---

# Phase 0 — bootstrap and provenance

## 0.1 Import Felucca foundation

- [ ] import the current Felucca firmware/tooling baseline while retaining all required copyright/license notices;
- [ ] preserve GPL-3.0-only license compatibility;
- [ ] preserve third-party SDK/license texts required by release packaging;
- [ ] record the exact upstream Felucca commit used as the initial base;
- [ ] create `UPSTREAM.md` describing divergence and update strategy.

## 0.2 Rebrand safely

- [ ] use `TheReFmB1rth` / a distinct product identity in UI and package metadata;
- [ ] no Roland or Propellerhead logos/artwork;
- [ ] no ReBirth skin recreation;
- [ ] classic machine names may be used only in documentation as descriptive references where appropriate.

## 0.3 Build baseline

- [ ] clean build on Linux;
- [ ] clean build using the JieLi AC79 toolchain;
- [ ] package generation works;
- [ ] browser emulator works;
- [ ] all inherited tests pass before feature changes.

Exit criterion: an unmodified/rebranded baseline builds and emulates successfully.

---

# Phase 1 — safe install/recovery infrastructure

This phase is required **before first experimental device flash**.

## 1.1 Installer preflight

Add `tools/preflight.py` and corresponding installer checks:

- [ ] enumerate MIDI devices;
- [ ] identify expected FM-1 identity;
- [ ] reject unknown devices;
- [ ] read current firmware/package identity where supported;
- [ ] verify `.fwsc` magic/header/size;
- [ ] SHA-256 package verification;
- [ ] require explicit `--experimental` for development firmware;
- [ ] require explicit typed confirmation for first experimental flash;
- [ ] never choose a MIDI output automatically if multiple possible FM-1 devices are present;
- [ ] refuse flashing when package identity does not match FM-1 target;
- [ ] log every update stage and received acknowledgement.

## 1.2 Package safety

- [ ] deterministic build artifact naming;
- [ ] `SHA256SUMS` for every release artifact;
- [ ] release manifest with upstream commit + local commit + build options;
- [ ] verify app image fits linker XIP bounds;
- [ ] verify RAM/pool symbols fit linker memory map;
- [ ] prevent package creation on overflow;
- [ ] prevent release packaging if tests fail.

## 1.3 Recovery documentation

- [ ] document return-to-official-firmware procedure;
- [ ] document boot-mode symptoms;
- [ ] document FM-1-transporter as last-resort recovery;
- [ ] document required backup steps before experimental testing;
- [ ] explicitly state that no firmware can guarantee zero brick risk.

Exit criterion: installer and package validation can reject intentionally corrupted packages in automated tests.

---

# Phase 2 — sequencer core

Create machine-independent sequencing first.

## Pattern model

Each acid track:

- pitch
- gate
- tie
- slide
- accent
- octave/transposition
- probability
- micro-timing offset
- optional per-step parameter locks

Each drum track:

- up to 11 core drum voices
- per-step trigger
- accent/velocity
- probability
- flam/ratchet
- micro-timing
- optional per-step parameter locks

Global:

- 16-step native page
- 32/48/64-step extended patterns
- pattern A–H initially
- copy/paste/clear
- pattern chaining
- song mode
- swing
- external MIDI clock
- internal MIDI clock
- start/stop/continue

Tests:

- [ ] exact event ordering;
- [ ] swing timing;
- [ ] slide continuity;
- [ ] accent timing;
- [ ] pattern wrap;
- [ ] pattern change quantization;
- [ ] clock drift over long runs.

---

# Phase 3 — ACID engine

Build one high-quality monophonic acid synth and instantiate it twice.

## Signal path

```text
oscillator
  -> pre-filter drive
  -> resonant nonlinear low-pass filter
  -> accent/envelope modulation
  -> VCA
  -> track drive
```

## Parameters

- waveform: saw / square
- tune
- cutoff
- resonance
- envelope modulation
- decay
- accent
- drive
- level
- pan

## Behaviour to model carefully

- [ ] monophonic note priority;
- [ ] oscillator phase continuity;
- [ ] slide is continuous rather than retriggered portamento;
- [ ] envelope retrigger rules depend on tied/slid steps;
- [ ] accent modifies amplitude and filter behaviour;
- [ ] resonance/cutoff interaction;
- [ ] useful self-oscillation approximation if CPU budget allows;
- [ ] nonlinear saturation around filter path;
- [ ] stable fixed-point implementation;
- [ ] no denorm/NaN/overflow conditions.

## Quality modes

Profile before deciding on oversampling:

- ECO: 1x
- NORMAL: 2x if affordable
- HQ: optional 4x only if the FM-1 CPU budget proves sufficient

Do not make oversampling a release requirement if it threatens real-time stability.

Exit criterion: two ACID engines + sequencer run simultaneously under target CPU budget.

---

# Phase 4 — 808 drum machine

Preferred implementation order:

1. synthesized voices where practical;
2. CC0/Public Domain samples only where this materially improves sound/CPU use;
3. no proprietary ROM/sample dumps.

Core voices:

- BD
- SD
- CP
- RS/CL
- CH
- OH
- LT
- MT
- HT
- CY
- CB

Per-voice controls should mimic the useful musical ranges rather than blindly duplicating vintage panel labels.

Example synthesis methods:

- kick: decaying sine + pitch envelope + transient
- snare: two resonant tonal components + filtered noise
- clap: multi-burst noise envelope
- hats/cymbal: multiple square/metal oscillators + high-pass/band-pass shaping
- toms: resonant decaying oscillators with pitch envelopes
- cowbell: paired pulse/square oscillators + envelope

Exit criterion: full 808-style pattern plays without samples except cymbal/hats if samples prove preferable.

---

# Phase 5 — 909 drum machine

Core voices:

- BD
- SD
- RS
- CP
- CH
- OH
- LT
- MT
- HT
- CR
- RD

Implementation strategy:

- synthesize kick/snare/toms/clap/hat-like components where practical;
- use openly licensed sample assets for crash/ride if synthesis is not convincing;
- preprocess every bundled asset to target sample rate/bit depth and remove unnecessary silence;
- document source URL, source license, original hash and processed hash.

Exit criterion: 909 + 808 + both acid engines run together at worst-case trigger density with no audio underruns.

---

# Phase 6 — sample asset policy and tooling

## Approved research candidates

### 808

Preferred source candidate:

- Michael Fischer / Edward Loveall TR-808 set as distributed by open-source music projects under Public Domain / CC0 lineage.

Do not assume a random mirror is safe. Verify the repository license and provenance before importing actual audio files.

### 909

Preferred source candidate:

- Oramics `TR-909 Detroit`, listed as public-domain/open licensed by the Oramics sampled collection.

Alternative/preferred for firmware where practical:

- synthesize original 808/909-inspired voices from documented circuit/DSP principles;
- CC0 `octalmusic/octal-samples` is useful reference/provenance research because its shipped sounds are synthesized from scratch or derived from verified public-domain material and released CC0.

## Tooling

Create `tools/fetch_samples.py` that:

- downloads only pinned URLs/commit hashes;
- validates SHA-256 of source files;
- converts to mono/stereo as required;
- resamples to 44.1 kHz;
- normalizes only where explicitly configured;
- trims silence conservatively;
- emits C/asset format expected by firmware;
- writes processed hashes;
- never downloads non-redistributable SampleRadar-style packs into release artifacts.

Create `assets/samples-cc0/ATTRIBUTION.md` with per-file provenance.

Release builds must fail if an asset has no declared license/provenance entry.

---

# Phase 7 — mixer and effects

Mixer per machine:

- level
- pan
- mute
- solo
- send A
- send B

Master/performance effects:

- distortion
- compressor
- tempo delay
- optional lightweight reverb if CPU budget permits
- performance filter/PCF-style sweep
- master limiter

Performance shortcuts:

- stutter/repeat
- filter sweep
- tape stop if affordable
- mute groups

All effects must have CPU regression tests.

---

# Phase 8 — FM-1 physical UI

The FM-1 should feel like a hardware groovebox, not a desktop UI squeezed onto 240×240.

## Home

- BPM
- current pattern/song position
- ACID1 / ACID2 / 808 / 909 status
- mute states
- transport

## Acid edit page

Four primary knobs per page:

1. cutoff
2. resonance
3. env mod
4. decay

secondary page:

1. accent
2. drive
3. tune
4. level

## Acid step page

- keyboard LEDs represent step activity/playhead;
- selected step shows note/octave/accent/slide;
- physical keys edit steps;
- modifiers edit accent/slide/tie.

## Drum page

- select instrument with encoder/knob;
- keys become 16-step programmer;
- LED brightness conveys normal/accent/current playhead;
- second page edits tuning/decay/tone/level.

## Mixer page

- machine levels
- mute/solo
- FX sends

## Song page

- pattern chain
- repeat count
- insert/delete/copy

---

# Phase 9 — MIDI

Default mapping:

- CH1: ACID 1
- CH2: ACID 2
- CH10: selected drum machine / GM-style drum input
- optional CH3: 808
- optional CH4: 909

Required:

- MIDI note in
- MIDI clock in/out
- start/stop/continue
- program/pattern change option
- CC mapping for major controls
- panic/all-notes-off
- USB + TRS paths

Optional later:

- compact SysEx editor protocol
- full browser editor
- pattern backup/restore

---

# Phase 10 — browser emulator

The emulator is a release gate, not a convenience feature.

Required:

- same DSP source as firmware;
- same sequencer source;
- same UI drawing code where practical;
- simulated FM-1 buttons/keys/knobs;
- WebAudio output;
- pattern/project persistence in browser;
- keyboard MIDI-style input;
- deterministic render tests.

Add a CPU approximation/per-block instrumentation view for development.

---

# Phase 11 — web editor

After firmware behaviour stabilizes:

- React + TypeScript
- machine rack view
- two acid panels
- 808/909-style grid without copied trademarked artwork
- pattern editor
- song arranger
- mixer
- automation editor
- project backup/restore
- Web MIDI direct FM-1 connection
- offline virtual mode using the same WASM DSP where possible

This editor is optional for the first hardware milestone.

---

# Phase 12 — persistence

Store:

- machine patches
- patterns
- songs
- mixer state
- FX state
- global settings

Requirements:

- versioned project format;
- CRC/checksum;
- atomic write strategy where possible;
- old-format migration tests;
- never overwrite loader/reserved flash sectors;
- corrupted project data must not prevent boot.

---

# Phase 13 — testing matrix

## DSP

- golden render hashes
- silence/DC checks
- clipping checks
- stress patterns
- fixed-point overflow tests
- filter stability tests

## CPU

Worst-case benchmark:

- ACID1 continuous resonant slide
- ACID2 continuous resonant slide
- all 808 voices active
- all 909 voices active
- distortion + compressor + delay + limiter
- UI update
- incoming MIDI clock

Keep headroom; do not ship at 99–100% theoretical budget.

## Storage

- interrupted write simulation
- corrupted CRC
- invalid format version
- full pattern banks

## Installer

- wrong MIDI device
- disconnected device
- corrupt package
- truncated package
- wrong product ID
- timeout at every update phase
- retry without destructive duplicate erase

---

# Phase 14 — first hardware bring-up

The first hardware milestone should be deliberately minimal.

Build profile:

- one ACID engine
- no sample assets
- no experimental flash layout changes
- inherited Felucca update loader
- simple UI
- audio output
- USB MIDI

Bring-up sequence:

1. run complete host test suite;
2. run emulator;
3. verify linker/map report;
4. generate package + SHA256;
5. run installer `--info` only;
6. confirm recovery resources are available;
7. flash one known test FM-1;
8. verify boot/display/audio/buttons/USB;
9. return to official firmware/Felucca and verify recovery path;
10. only then expand device tests.

Never test a new loader and new application image simultaneously.

---

# Initial milestone order

## M0 — safe baseline
- Felucca base imported
- licensing/provenance preserved
- clean build
- emulator works
- tests green

## M1 — Acid prototype
- one acid engine
- 16-step note/accent/slide sequencer
- emulator demo
- golden DSP tests

## M2 — Twin acid
- two independent acid engines
- mixer
- MIDI channels 1/2

## M3 — 808
- synthesized 808-style kit
- drum step UI

## M4 — 909
- synthesized/sample hybrid 909-style kit
- approved open assets only

## M5 — complete groovebox
- song mode
- mixer
- effects
- save/load
- MIDI sync

## M6 — hardware beta
- safe installer preflight
- package verification
- documented recovery
- experimental hardware test

## M7 — editor
- Web MIDI editor
- offline WASM instrument

---

# Definition of done for v1.0

- [ ] 2 acid machines playable and sequenced independently;
- [ ] 808 and 909 machine sections run simultaneously;
- [ ] stable 44.1 kHz output with comfortable CPU headroom;
- [ ] pattern + song modes;
- [ ] swing, accent, slide, probability;
- [ ] mixer and essential effects;
- [ ] USB MIDI + TRS MIDI clock/input;
- [ ] USB audio still works;
- [ ] save/load projects;
- [ ] browser emulator;
- [ ] complete host/regression test suite;
- [ ] all bundled samples have auditable open licenses;
- [ ] release includes hashes and provenance;
- [ ] safe installer refuses mismatched/corrupt packages;
- [ ] documented recovery/return-to-official path;
- [ ] no known flash/RAM/stack/CPU budget violations.

---

# Immediate next tasks

1. Import/pin Felucca upstream foundation and preserve its license tree.
2. Add `docs/FLASH_SAFETY.md` and `docs/SAMPLE_SOURCES.md`.
3. Reproduce a clean Felucca build before changing DSP.
4. Add CI for host tests and artifact-size checks.
5. Implement machine-neutral 16-step pattern representation.
6. Implement the first ACID oscillator/envelope/filter prototype on host/WASM.
7. Add deterministic acid DSP golden tests.
8. Only after emulator validation, begin FM-1 package testing.
