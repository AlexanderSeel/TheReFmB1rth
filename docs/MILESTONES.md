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
- [x] hardware-neutral `refm_target` ABI for render/MIDI/transport/persistence
- [x] real fixed-point ACID amp ADSR with attack/decay/sustain/release stages
- [x] real fixed-point ACID LFO with sine-like/triangle/saw/square shapes and filter modulation
- [x] shared 64-point ADSR/LFO/filter graph models used by target UI and browser/WASM path
- [x] physical Felucca ENV page edits the selected ACID engine ADSR and renders its ReFmB1rth curve
- [x] physical Felucca LFO page edits the selected ACID engine LFO and renders its ReFmB1rth curve
- [x] physical Felucca LFO filter destination drives the ReFmB1rth filter-modulation depth
- [x] physical Felucca EDIT 1 page is bridged to ACID cutoff/resonance/env-mod/decay and renders the ReFmB1rth filter response curve
- [x] WASM C bridge using the exact same `refm_target` runtime
- [x] reproducible Emscripten build script for the shared engine
- [x] GitHub Actions builds and uploads `refm.js` + `refm.wasm` with SHA-256 checksums
- [x] native host compilation/tests for target ABI and WASM bridge
- [x] automated integration tests for the unified render path
- [x] CI verifies the exact pinned Felucca audio/build integration anchors
- [x] deterministic overlay patches Felucca `audio_block()` to the ReFmB1rth Q15 renderer
- [x] Felucca USB and TRS MIDI converge through `midi_enqueue()` and are mirrored into the ReFmB1rth MIDI path
- [x] Felucca update loader remains independent of the ReFmB1rth app MIDI mirror
- [x] ReFmB1rth is compiled as isolated target objects to avoid Felucca/internal C namespace collisions
- [x] freestanding FM-1 target build has a local `string.h` shim instead of depending on an unavailable target libc
- [x] ReFmB1rth project slots are wired to Felucca's CRC-checked A/B atomic storage mechanism
- [x] copied target sources compile cleanly with strict host warnings in CI after the overlay is applied
- [x] complete pinned JieLi/AC79 target compile + link succeeds in GitHub Actions
- [x] large ReFmB1rth runtime/project work buffers are placed in Felucca's dedicated `.pool` region rather than exhausting 96 KiB general RAM
- [x] GitHub target build emits machine-readable XIP/RAM/pool usage and refuses release artifacts below conservative headroom thresholds

**M5 target integration still open:**

- [ ] expose the ReFmB1rth project save/load hooks through the physical FM-1 UI
- [ ] finish dedicated labels/layout and secondary ACID edit page instead of reusing generic Felucca EDIT card labels
- [ ] implement the physical FM-1 mixer/song/pattern/drum UI and complete ACID step editing
- [ ] add the browser UI/audio-worklet shell around the WASM bridge
- [ ] add deterministic audio render hashes to CI
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
- [ ] the browser emulator UI passes with the integrated runtime
- [ ] worst-case CPU/audio-underrun gates pass
- [ ] runtime stack watermark/headroom is measured under worst-case UI + audio + MIDI activity
- [ ] return-to-stock and FM-1-transporter recovery preparation is confirmed for the test unit
- [ ] a controlled physical FM-1 beta installation is performed successfully
- [ ] post-install USB MIDI, TRS MIDI, USB audio, storage and recovery are verified on hardware

A successful GitHub build proves reproducibility and target-toolchain compatibility; it does **not** by itself make the artifact safe to flash. A physical flash is deliberately never performed by CI. There is no zero-brick guarantee.
