# Milestone implementation status

## M5 — complete groovebox

**Implemented and covered by CI:**

- [x] two ACID engines in a shared runtime
- [x] original 808-inspired and 909-inspired synthesized drum engines
- [x] 11 drum lanes: BD, SD, clap, rim, closed/open hats, low/mid/high toms, crash, ride
- [x] song chain model with per-slot repeats and looping
- [x] A–H pattern-bank storage, clear, copy, capture and load helpers
- [x] all A–H pattern-bank data is persisted in the ReFmB1rth project payload
- [x] song changes load the corresponding A–H bank on the quantized 16-step boundary
- [x] four-track mixer with level/pan/mute/solo
- [x] per-track delay sends
- [x] master drive, compressor, delay, performance low-pass and bounded output
- [x] MIDI 24 PPQN START/CONTINUE/STOP/CLOCK transport
- [x] integer internal BPM clock
- [x] MIDI note/CC/program routing for both acid tracks and both drum machines
- [x] channel-10 drum routing plus dedicated drum channels
- [x] raw MIDI byte parser with running status and realtime-message interleaving
- [x] versioned project envelope + CRC32 corruption rejection
- [x] groovebox state save/load for sequencers, A–H banks, drum grids, mixer/FX, song data and ACID modulation settings
- [x] backward-compatible optional project extension for ACID ADSR/LFO settings
- [x] backward-compatible optional project extension for ACID oscillator tune
- [x] hardware-neutral `refm_target` ABI for render/MIDI/transport/persistence
- [x] TB-303-focused fixed-point ACID path with saw/asymmetric-square VCO, legato slide, filter envelope, accent sweep memory and stock-style VCA contour
- [x] Open303-informed four-stage TeeBeeFilter topology with resonance skew, nonlinear feedback and ~150 Hz feedback high-pass
- [x] band-limited PolyBLEP correction on the saw and asymmetric square discontinuities
- [x] 2x internal oscillator/filter processing at 88.2 kHz with output down-average
- [x] stock-style TUNE control implemented in the DSP and exposed through WASM
- [x] deterministic groovebox render fingerprint locked to the improved TB-303 DSP path
- [x] optional fixed-point ACID LFO with sine-like/triangle/saw/square shapes and filter modulation; amount defaults to zero and is treated as a non-stock MOD extension
- [x] shared 64-point envelope/LFO/filter graph models used by target UI and browser/WASM path
- [x] physical Felucca EDIT page is bridged to ACID cutoff/resonance/env-mod/decay and renders the ReFmB1rth filter response curve
- [x] compact FM-1 ReBirth-inspired ACID step-lamp overlay exists without copied proprietary artwork/assets
- [x] WASM C bridge using the exact same `refm_target` runtime
- [x] reproducible Emscripten build script for the shared engine
- [x] standalone `web/emu/index.html` WASM lab for browser testing without the final React application
- [x] browser lab provides WebAudio playback, transport, ACID keyboards and live engine status
- [x] browser lab edits real A–H pattern memory
- [x] browser lab exposes both 16-step ACID sequencers with gate/accent/slide/tie states
- [x] browser lab exposes 808 and 909 11-lane × 16-step drum grids with hit/accent editing and live playhead
- [x] ReBirth-inspired original browser skin separates STOCK 303 controls from the optional EXT / MOD section
- [x] source-level UI contract tests guard the ReBirth-inspired skin, TUNE export and MOD separation
- [x] WASM ABI exposes pattern select, acid-step get/set and drum-step get/set rather than duplicating sequencer state in JavaScript
- [x] GitHub Actions performs a browser-asset/export smoke check before publishing the WASM artifact
- [x] redistributable sample allow-list exists in `samples/sources.json`
- [x] initial 808 sample source is pinned to the Fischer/Loveall CC0 repository with per-file Git blob identities
- [x] initial 909-style sample source is pinned to the Octal CC0 kit with per-file Git blob identities
- [x] deterministic PCM16 preprocessing/downmix/trim/resample pipeline generates auditable C/PCM assets and per-asset SHA-256 metadata
- [x] allocation-free hybrid drum engine supports per-lane sample playback with synthesized fallback and sample open-hat choking
- [x] sample-backed WASM audition build can A/B 808 and 909 synth/sample paths without embedding samples in the FM-1 image
- [x] conservative FM-1 sample candidate budget is measured separately from the full browser audition set
- [x] AudioWorklet timing lab exists alongside the main WASM lab
- [x] native host compilation/tests for target ABI and WASM bridge
- [x] deterministic overlay patches Felucca audio/MIDI/storage integration points
- [x] complete pinned JieLi/AC79 target compile + link succeeds in GitHub Actions
- [x] large ReFmB1rth runtime/project work buffers are placed in Felucca's dedicated `.pool` region rather than exhausting 96 KiB general RAM
- [x] GitHub target build emits machine-readable XIP/RAM/pool usage and refuses release artifacts below conservative headroom thresholds

**M5 target integration still open:**

- [ ] expose the ReFmB1rth project save/load hooks through the physical FM-1 UI
- [ ] finish dedicated FM-1 stock-303 labels/layout and secondary EXT/MOD page instead of reusing generic Felucca card labels
- [ ] implement the physical FM-1 mixer/song/pattern/drum UI and complete ACID step editing
- [ ] calibrate cutoff/env-mod/accent curves by listening and reference rendering against Open303/Digix0x-style references
- [ ] measure worst-case CPU/audio-underrun cost of the 2x ACID ladder on target hardware; keep a quality fallback only if the FM-1 cannot sustain the full path
- [ ] listen/measure and explicitly approve the small FM-1 sample candidate set; full browser sample kits must not be copied automatically into target flash
- [ ] embed only approved target sample candidates and re-run XIP/CPU/audio-underrun gates
- [ ] move WASM DSP production fully off main-thread scheduling (the current AudioWorklet sink is a buffered timing prototype, not the final Worker/Worklet architecture)
- [ ] build the full React editor around the validated WASM ABI
- [ ] add full four-machine CPU-budget/audio-underrun regression measurements on target-equivalent and physical builds
- [ ] add an explicit stack-watermark/runtime stack gate in addition to the static RAM/pool gate

## M6 — hardware beta

**Implemented safety/build tooling and covered by CI:**

- [x] deterministic release manifest with SHA-256/product/upstream provenance
- [x] package/device identity safety gate
- [x] explicit experimental and typed-confirmation requirements
- [x] guarded installer wrapper, dry-run by default
- [x] second acknowledgement before any write
- [x] installation delegated to pinned Felucca loader-mediated installer
- [x] recovery prerequisites documented
- [x] automated negative tests for tampering/wrong-device/missing-confirmation
- [x] Python safety tests execute under the actual CI `unittest` runner
- [x] pinned Felucca commit and source integration anchors validated on every CI run
- [x] overlay application is deterministic and refuses a non-pinned Felucca checkout
- [x] GitHub Actions downloads the official JieLi toolchain and pinned AC79 SDK
- [x] a real experimental TheReFmB1rth `.fwsc`, application `.bin` and Felucca loader `ota.bin` are produced entirely in GitHub Actions
- [x] the generated `.fwsc` passes manifest/product/SHA verification before artifact upload
- [x] explicit XIP/general-RAM/pool release gates are enforced before artifact upload
- [x] the target artifact contains `target-memory.json` plus linker symbols for auditability
- [x] target-side audio/MIDI/storage/UI integration remains behind the guarded overlay/build process; CI never flashes hardware

**M6 remains intentionally blocked from being called complete until:**

- [ ] inherited Felucca host/update/loader/emulator regression suites are run against the integrated overlay, not only the target build
- [ ] the browser emulator UI passes with the integrated runtime under final Worker/AudioWorklet timing
- [ ] worst-case CPU/audio-underrun gates pass, including the 2x ACID ladder path
- [ ] runtime stack watermark/headroom is measured under worst-case UI + audio + MIDI activity
- [ ] return-to-stock and FM-1-transporter recovery preparation is confirmed for the test unit
- [ ] a controlled physical FM-1 beta installation is performed successfully
- [ ] post-install USB MIDI, TRS MIDI, USB audio, storage and recovery are verified on hardware

A successful GitHub build proves reproducibility and target-toolchain compatibility; it does **not** by itself make the artifact safe to flash. A physical flash is deliberately never performed by CI. There is no zero-brick guarantee.
