# Milestone implementation status

## M5 — complete groovebox

**Implemented and covered by host CI:**

- [x] two ACID engines in a shared runtime
- [x] original 808-inspired and 909-inspired synthesized drum engines
- [x] 11 drum lanes: BD, SD, clap, rim, closed/open hats, low/mid/high toms, crash, ride
- [x] song chain model with per-slot repeats and looping
- [x] A–H pattern-bank storage, clear, copy, capture and load helpers
- [x] four-track mixer with level/pan/mute/solo
- [x] per-track delay sends
- [x] master drive, compressor, delay, performance low-pass and bounded output
- [x] MIDI 24 PPQN START/CONTINUE/STOP/CLOCK transport
- [x] integer internal BPM clock
- [x] MIDI note/CC/program routing for both acid tracks and both drum machines
- [x] channel-10 drum routing plus dedicated drum channels
- [x] raw MIDI byte parser with running status and realtime-message interleaving
- [x] versioned project envelope + CRC32 corruption rejection
- [x] groovebox state save/load for active sequencers, drum grids, mixer/FX and song data
- [x] hardware-neutral `refm_target` ABI for render/MIDI/transport/persistence
- [x] WASM C bridge using the exact same `refm_target` runtime
- [x] reproducible Emscripten build script for the shared engine
- [x] native host compilation/tests for target ABI and WASM bridge
- [x] automated integration tests for the unified render path
- [x] CI verifies the exact pinned Felucca audio/build integration anchors

**M5 target integration still open:**

- [ ] store all A–H pattern-bank data in the production project format, not only active runtime state
- [ ] connect quantized song changes directly to pattern-bank loading
- [ ] integrate `refm_target_render_q15()` into Felucca's real `audio_block()`/I2S path
- [ ] bind project save/load to Felucca flash sectors atomically
- [ ] feed Felucca USB/TRS MIDI byte sources into `refm_target_midi_byte()`
- [ ] implement the physical FM-1 mixer/song/pattern UI
- [ ] add the browser UI/audio-worklet shell around the new WASM bridge
- [ ] build the Emscripten artifact in CI and add deterministic render hashes
- [ ] add full four-machine CPU-budget/underrun regression measurements on target-equivalent builds

## M6 — hardware beta

**Implemented safety tooling and covered by CI:**

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

**M6 remains intentionally blocked from being called complete until:**

- [ ] a real TheReFmB1rth target `.fwsc` is produced from the pinned Felucca foundation
- [ ] inherited Felucca build/update/loader tests pass with the integrated runtime
- [ ] the WASM emulator passes with the integrated runtime and UI
- [ ] XIP/RAM/pool/stack map gates pass
- [ ] worst-case CPU/audio-underrun gates pass
- [ ] manifest/package verification passes on the real artifact
- [ ] return-to-stock and FM-1-transporter recovery preparation is confirmed
- [ ] a controlled physical FM-1 beta installation is performed successfully

A physical flash is deliberately not performed by CI or by repository tooling automatically. There is no zero-brick guarantee.
