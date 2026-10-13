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

## Next interaction milestone — keyboard-first live workflow

The next UI pass should make much stronger use of the FM-1 keyboard and treat the keys as a contextual performance/edit surface rather than only as note keys. The physical hardware and virtual FM-1 must expose the same mappings so virtual rehearsal matches the real unit.

### Keyboard-assisted note and drum entry

- On ACID pages, the keyboard should enter notes directly into the selected sequencer step.
- In step-edit mode, pressing a key writes the pitch immediately; holding a modifier button while pressing a key may assign accent, slide, tie or probability-related variants without navigating through extra pages.
- A dedicated live-record mode should allow notes played on the keyboard to be captured into the running pattern with sensible quantization.
- On DRUM pages, the keyboard should become a drum-pad bank. Individual keys should map to BD, SD, clap, hats, toms, cymbals and the remaining 808/909 voices.
- Drum live-record should write hits directly into the running step position. Accent/high-level and 909 flam states should be accessible through modifiers rather than requiring deep menu navigation.
- The 808 and 909 mappings should remain visibly different where the machines differ: 909 flam/high-low behavior versus the broader 808 percussion voice set.

### Live performance mode

A dedicated live-performance mode should keep the most useful performance actions one gesture away while the sequencer is running.

- Keyboard notes remain immediately playable for ACID A/B.
- Selected keyboard keys may temporarily become performance controls when a modifier is held.
- Quick access should exist for filter cutoff/resonance, delay, PCF, distortion and compressor/performance effects.
- Mute/solo for ACID A, ACID B, 808 and 909 should be available without entering the mixer page.
- Pattern A–H switching should be possible from the keyboard and should respect the configured quantized-change behavior.
- One-shot/special-effect actions should be possible from spare keys, for example delay throw, filter sweep, temporary distortion, repeat/stutter, mute cut or similar performance-safe effects.
- Entering live play should be fast, and leaving it should restore the normal keyboard mapping immediately.

### Hold/modifier behavior

Physical buttons should gain a consistent short-press/hold language. A short press keeps the existing primary action. Holding a button temporarily exposes a second layer on the keyboard or encoders.

Examples:

- hold `SEQ` + keyboard: select/write sequencer steps or notes;
- hold `ARP/DRUM` + keyboard: choose or trigger drum voices;
- hold `GLO/MIXER` + keyboard: mute/solo tracks and select mixer channels;
- hold `FX` + keyboard: select PCF/Delay/Dist/Comp and trigger performance FX;
- hold `PRESET` or a dedicated pattern modifier + keyboard: choose patterns A–H;
- hold `EDIT` + keyboard: accent/slide/tie/flam or other context-specific edit layer;
- hold `HOME` should always provide a predictable escape/back behavior and must never leave stuck notes.

Modifier mappings must be contextual, visible on the 240×240 screen, and designed so accidental mode changes are difficult during live use.

### Quick panel zoom/navigation mode

The 240×240 display should support a quick zoom/focus mode for dense ReBirth-inspired panels. This is not a graphical magnification effect only; it is a navigation mode for temporarily focusing on one logical panel.

- A short dedicated gesture should enter `ZOOM`/`FOCUS` mode from HOME, ACID, STEP, DRUM, MIXER or FX.
- The keyboard can act as direct panel selectors while zoom mode is active.
- Keys may select regions such as oscillator/filter, sequencer, drum voices, mixer, PCF, delay, distortion and compressor.
- Once zoomed, the four K encoders should control the most important parameters of that focused panel.
- PRESET/ALGORITHM/SELECT or spare keyboard keys can move left/right/up/down between adjacent logical panels.
- The display should show a small overview/location indicator so the user always knows which part of the larger virtual panel is currently focused.
- HOME or the same zoom gesture exits immediately.
- Virtual mode must mirror the same focus state and navigation so it is useful for hardware training and testing.

### Interaction design rules

The goal is speed and muscle memory, not adding hidden complexity. Every secondary mapping must be shown on-screen when its modifier is held. Common live actions should require at most one held modifier plus one key. Note-off/panic behavior must remain safe across page, machine, pattern and mode changes. The keyboard mapping should be consistent between physical FM-1, virtual FM-1 and Live Editor wherever possible.

The implementation should be validated against realistic tasks: enter a bassline without leaving STEP mode, record a drum beat live, switch A–H patterns during playback, mute one machine, perform a filter/delay gesture, jump into a zoomed FX panel, edit it with K1–K4, and return to live notes without stopping playback.

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
