# TheReFmB1rth

Experimental replacement firmware project for the M-VAVE FM-1: a compact four-machine acid groovebox inspired by the classic two-bassline/two-drum-machine workflow.

## Target

- ACID A — original 303-inspired mono synth
- ACID B — second independent acid synth
- 808 — original synthesized drum machine
- 909 — second synthesized drum machine with real sequencer flam state
- A–H patterns + song chaining
- four-track mixer + performance FX
- USB/TRS MIDI and USB audio
- browser installer/editor + Virtual FM-1/WASM
- guarded FM-1 installer based on the pinned Felucca update path

See [`plan.md`](plan.md), [`docs/MILESTONES.md`](docs/MILESTONES.md), [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) and [`docs/M6_HARDWARE_BETA.md`](docs/M6_HARDWARE_BETA.md).

## Current status

**`v0.1.0-beta.3` is the current published hardware-test baseline. Current `main` contains unreleased post-beta.3 usability work and must not be treated as hardware-stable yet.**

The engine/runtime foundation is implemented software-side and real FM-1 `.fwsc` packages build in GitHub Actions. Physical validation has also exposed important blockers: the earlier hardware build booted and rendered the LCD, but practical sequence editing, live play/audio and several dense pages were not yet usable. The current work therefore prioritizes basic groovebox usability over adding more advanced features.

Implemented foundations include:

- two acid engines and two synthesized 11-lane drum engines in one runtime;
- 16-step sequencing with accent/slide/tie/probability;
- A–H pattern persistence, quantized live pattern switching and song-chain changes;
- level/pan/mute/solo mixer, delay/reverb sends, drive, compressor and performance low-pass;
- internal clock plus MIDI START/CONTINUE/STOP/CLOCK;
- MIDI note/CC/program routing;
- versioned CRC-protected project serialization and Felucca A/B atomic flash integration;
- shared target/WASM runtime and shared `ui240` state model;
- Felucca Q15 audio callback integration plus shared USB/TRS MIDI ingress;
- release manifests, SHA/product/device checks and guarded release publishing;
- CI tests for DSP, sequencer, runtime, pattern/MIDI, projects and hardware-gate logic.

## Virtual FM-1: simple sequencing first

The Virtual FM-1 no longer requires modifier chords for normal sequence programming.

### ACID A / ACID B

Use the **SEQ** page as a direct 303-style step editor:

1. select one of the 16 steps;
2. choose the note, REST or TIE;
3. toggle ACCENT and SLIDE directly;
4. audition/play through the same groovebox runtime used by the target build.

The LCD now receives all 16 real ACID steps from WASM. REST/NOTE/TIE, accent and slide are rendered from actual sequencer state instead of being inferred from the selected step.

### 808 / 909

Select a drum lane and click the 16-step grid. Each cell cycles:

**OFF → HIT → ACCENT → OFF**

The selected lane's actual hit/accent state is displayed. 909 flam is kept as real persisted sequencer state rather than a cosmetic browser flag.

### 2× display

The logical display remains **240×240**, matching the physical FM-1. The Virtual tab also provides a true pixel-perfect **480×480 DISPLAY 2×** view, including direct screen editing.

## New TheReFmB1rth startup sequence

Current `main` adds an original startup/loading sequence for both Virtual FM-1 and the physical target concept:

- **THE ReFM B1RTH / RB-240** identity;
- ACID A → ACID B → 808 → 909 machine activation;
- animated 16-step bus;
- final READY state;
- industrial metal/LED visual language inspired by classic groovebox software without copying protected logos or artwork.

The physical implementation uses existing 240×240 LCD/text primitives and incremental phase updates. Audio, input, USB and timer initialization occur before the animated main-loop phases, and the previous additional blocking Felucca splash delay is removed from the generated target path.

## Physical UI cleanup in progress

A legacy Felucca compatibility graph was still being injected underneath the dedicated RB-240 display and could appear as an unwanted horizontal/pattern strip across unrelated pages. Current `main` removes that injection: the dedicated ReFm renderer now owns normal LCD content.

This does **not** mean the physical UI is finished. The next hardware pass must verify:

- no remaining graph/renderer bleed-through;
- live keyboard notes are audible;
- PLAY starts the seeded Pattern A audibly;
- ACID step editing is fast and predictable;
- 808/909 lane + step editing is fast and predictable;
- parameter changes do not cause distracting full-screen redraws;
- labels correspond to the controls actually used;
- unnecessary persistent bottom help bands are removed.

The host integration tests already prove that direct MIDI and the seeded Pattern A can generate non-zero ReFm audio through the Felucca bridge. If the next hardware test still has silence, investigation should therefore isolate the physical event/audio/DAC chain rather than blindly changing working DSP.

## Advanced keyboard/performance interaction

The shared `ui240_performance` layer still supports performance shortcuts on physical and Virtual FM-1, but these are considered advanced controls rather than the primary sequencing workflow.

| Hold | Temporary behavior |
| --- | --- |
| `SEQ` | selected-step ACID note entry |
| `ARP/DRUM` | 808/909 drum-entry bank |
| `GLO/MIXER` | mute / solo / machine selection |
| `FX` | PCF / Delay / Dist / Comp selection and performance throws |
| `SAVE` | direct Pattern A–H selection |
| `EDIT` | contextual accent/slide/tie/rest/probability shortcuts |
| `REC` | live capture into the running step |
| `SCL/SEL` | FOCUS navigation |

`HOME` remains the deterministic escape/panic action. These mappings may still be simplified on hardware if they interfere with basic step programming.

## Pattern switching during playback

Direct Pattern A–H selection is immediate while transport is stopped. During playback, a requested pattern is queued and applied on the next 16-step boundary. The current pattern is stored at the switch point so live edits are not lost.

## FX and mixer routing

Each machine has level, pan, mute, solo, delay send and reverb send. Dry/return signal then passes through the compact master performance rack: **Dist → Comp → PCF**. Browser builds may use larger effect buffers; the FM-1 target keeps conservative buffers until physical profiling proves more memory/CPU headroom.

## Build everything in GitHub

`.github/workflows/build-all.yml` builds on every push to `main` and can also be started manually from GitHub Actions.

It produces:

1. **`refm-wasm-<commit>`**
   - `refm.js`
   - `refm.wasm`
   - combined browser app including Virtual FM-1
   - `SHA256SUMS`

2. **`refm-fm1-experimental-<commit>`**
   - `TheReFmB1rth-experimental.fwsc`
   - application binary
   - Felucca `ota.bin`
   - `manifest.json`
   - overlay provenance/memory data
   - `SHA256SUMS`

The FM-1 job prepares the exact pinned Felucca 1.0.5 checkout, applies the deterministic TheReFmB1rth overlay, downloads the JieLi pi32v2 toolchain, fetches the pinned AC79 SDK, performs the real target compile/link/package build, enforces memory gates and only then uploads artifacts. The workflow never connects to or flashes a physical FM-1.

Useful checks:

```sh
tests/run_host_tests.sh
sh web/emu/build_wasm.sh
# Real target package: use Build all artifacts in GitHub Actions.
```

## Release policy

Published beta releases are immutable. **Do not repoint or overwrite `v0.1.0-beta.3`.** Current `main` remains unreleased until host, WASM and real target package gates are green on one exact commit and the new physical startup/UI/audio/basic sequencing workflow has been tested on hardware. A new release is created only by an explicit release request.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes most 808/909-style voices and uses CC0/Public Domain samples only where useful. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are historical references to instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from Felucca-derived source and all included third-party components.
