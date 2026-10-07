# TheReFmB1rth

Experimental replacement firmware project for the M-VAVE FM-1: a compact four-machine acid groovebox inspired by the classic two-bassline/two-drum-machine workflow.

## Target

- ACID 1 — original 303-inspired mono synth
- ACID 2 — second independent acid synth
- DRUM A — original 808-inspired synthesized drum machine
- DRUM B — original 909-inspired synthesized drum machine
- A–H patterns + song chaining
- mixer + performance FX
- USB/TRS MIDI and USB audio
- browser/WASM emulator
- guarded FM-1 installer based on the proven Felucca update path

See [`plan.md`](plan.md) and [`docs/MILESTONES.md`](docs/MILESTONES.md).

## Current status

**M5 groovebox functionality, the Felucca integration overlay and the reproducible GitHub build pipeline are implemented. A real experimental FM-1 `.fwsc` now builds successfully in GitHub Actions. This is still not a hardware release: do not flash the artifact until the linker-budget, CPU/underrun, emulator and recovery gates are completed.**

Implemented foundations include:

- two acid engines and two synthesized 11-lane drum engines in one four-machine runtime;
- 16-step sequencing with accent/slide/tie/probability primitives;
- complete A–H pattern persistence and quantized song-chain pattern changes;
- level/pan/mute/solo mixer, delay sends, drive, compressor, delay, performance low-pass and bounded master output;
- internal clock plus MIDI START/CONTINUE/STOP/CLOCK;
- MIDI note/CC/program routing for acid and drum machines;
- versioned CRC-protected project serialization and Felucca A/B atomic flash integration;
- LFO/filter/ADSR graph models;
- shared target/WASM runtime;
- Felucca Q15 audio callback integration plus shared USB/TRS MIDI ingress;
- release manifests, SHA/product/device checks and a double-confirmed experimental installer wrapper that delegates to the pinned Felucca loader-mediated installer;
- CI tests for DSP, sequencer, runtime, pattern/MIDI, projects and hardware-gate logic.

## Build everything in GitHub

`.github/workflows/build-all.yml` builds the project on every push to `main` and can also be started manually with **Actions → Build all artifacts → Run workflow**.

It produces two downloadable Actions artifacts:

1. **`refm-wasm-<commit>`**
   - `refm.js`
   - `refm.wasm`
   - `SHA256SUMS`

2. **`refm-fm1-experimental-<commit>`**
   - `TheReFmB1rth-experimental.fwsc`
   - `TheReFmB1rth-app.bin`
   - Felucca `ota.bin`
   - `manifest.json`
   - overlay provenance stamp
   - `SHA256SUMS`

The FM-1 job reproducibly prepares the exact pinned Felucca 1.0.5 checkout, applies the deterministic ReFmB1rth overlay, downloads the official JieLi pi32v2 toolchain, fetches the required AC79 SDK, performs the actual target compile/link/package build, verifies the generated firmware manifest, and only then uploads the artifact. **The workflow never connects to or flashes a physical FM-1.**

The first fully successful all-GitHub target/WASM build was produced from commit `ba6070b26af7b313929a3e4778c21496631d75f0`.

## Remaining release gates

Before a controlled hardware beta, the repository still needs explicit XIP/RAM/pool/stack budget enforcement, worst-case CPU/audio-underrun profiling, the complete physical FM-1 UI, browser UI/audio-worklet integration, integrated emulator regression coverage and confirmed return-to-stock/FM-1-transporter recovery preparation.

Read [`docs/FLASH_SAFETY.md`](docs/FLASH_SAFETY.md) and [`docs/M6_HARDWARE_BETA.md`](docs/M6_HARDWARE_BETA.md) before any future hardware experiment.

## Sample policy

Only auditable redistributable content may be shipped. The preferred implementation synthesizes most 808/909-style voices and uses CC0/Public Domain samples only where useful, especially cymbals/ride/crash. See [`docs/SAMPLE_SOURCES.md`](docs/SAMPLE_SOURCES.md).

## Legal / identity

This is an independent open-source project. It is not affiliated with or endorsed by Roland, Propellerhead/Reason Studios, M-VAVE, or the Felucca authors. Product names mentioned in documentation are references to historical instruments/software and desired workflow/sound characteristics.

The firmware implementation must preserve licenses and notices from any Felucca-derived source and from all included third-party components.
