# Milestone implementation status

## M5 — complete groovebox

**Implemented as host-testable core:**

- [x] song chain model with per-slot repeats and looping
- [x] four-track mixer with level/pan/mute/solo
- [x] per-track delay sends
- [x] master drive, compressor, delay, performance low-pass and bounded output
- [x] MIDI 24 PPQN START/CONTINUE/STOP/CLOCK transport
- [x] integer internal BPM clock
- [x] versioned project envelope + CRC32 corruption rejection
- [x] automated host tests for the above

**Integration still open:** actual four-machine audio render, real flash storage binding, FM-1/WASM UI pages and USB/TRS MIDI routing.

## M6 — hardware beta

**Implemented safety tooling:**

- [x] deterministic release manifest with SHA-256/product/upstream provenance
- [x] package/device identity safety gate
- [x] explicit experimental and typed-confirmation requirements
- [x] guarded installer wrapper, dry-run by default
- [x] second acknowledgement before any write
- [x] installation delegated to pinned Felucca loader-mediated installer
- [x] recovery prerequisites documented
- [x] automated negative tests for tampering/wrong-device/missing-confirmation

**Not complete:** no TheReFmB1rth target `.fwsc` has yet passed the full Felucca build/emulator/linker/CPU gates, and no physical FM-1 beta flash has been performed. That remains intentionally blocked rather than being marked complete prematurely.
