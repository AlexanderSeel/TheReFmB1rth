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
- [x] generated pitch-banded Q15 wavetable oscillator with PolyBLEP reference/fallback build
- [x] 2x internal oscillator/filter processing at 88.2 kHz with output down-average
- [x] measured Open303 cutoff/env-mod law, ~15 ms envelope RC, accented timing and idle-only analog reset
- [x] js303/Open303-informed ~44.486 Hz pre-ladder high-pass, ~24 Hz post-ladder conditioning and two-stage ~200 Hz VCA gain de-click smoothing
- [x] expensive filter coefficient mapping runs at control rate with interpolation while the nonlinear ladder remains at 88.2 kHz
- [x] stock-style TUNE control implemented in the DSP and exposed through WASM
- [x] deterministic groovebox render fingerprint locked to `88cefa40afeecf47` for the current analog-conditioned path
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
- [x] browser rack provides an x0x-style ACID step programmer with note keyboard, octave, gate, accent, slide, tie and rest editing plus live audition
- [x] browser rack exposes separate always-visible 808 and 909 11-lane × 16-step drum machines with hit/accent editing and live playhead
- [x] original hardware-rack browser skin separates stock TB-style controls from optional EXT / MOD features without copied proprietary artwork
- [x] browser mixer rack controls the real four-track level/pan/mute/solo/delay-send engine state
- [x] browser MASTER FX rack controls the real drive, compressor, performance filter and delay engine state
- [x] source-level UI contract tests guard the hardware-rack skin, note programmer, dual drum machines, mixer/FX bridge, TUNE export and MOD separation
- [x] WASM ABI exposes pattern select, acid-step get/set and drum-step get/set rather than duplicating sequencer state in JavaScript
- [x] GitHub Actions performs a browser-asset/export smoke check before publishing the WASM artifact
- [x] redistributable sample allow-list exists in `samples/sources.json`
- [x] initial 808 sample source is pinned to the Fischer/Loveall CC0 repository with per-file Git blob identities
- [x] initial 909-style sample source is pinned to the Octal CC0 kit with per-file Git blob identities
- [x] deterministic PCM16 preprocessing/downmix/trim/resample pipeline generates auditable C/PCM assets and per-asset SHA-256 metadata
- [x] allocation-free hybrid drum engine supports per-lane sample playback with synthesized fallback and sample open-hat choking
- [x] per-lane sample availability, selection and active masks are explicit in the runtime/WASM ABI; browser lanes report SAMPLE/READY/SYNTH rather than silently falling back
- [x] browser WASM initializes bundled drum assets in hybrid mode so available selected lanes actually use samples by default
- [x] sample-backed WASM audition build can A/B 808 and 909 synth/sample paths without embedding samples in the FM-1 image
- [x] conservative FM-1 sample candidate budget is measured separately from the full browser audition set
- [x] AudioWorklet timing lab exists alongside the main WASM lab
- [x] native host compilation/tests for target ABI and WASM bridge
- [x] deterministic overlay patches Felucca audio/MIDI/storage integration points
- [x] complete pinned JieLi/AC79 target compile + link succeeds in GitHub Actions
- [x] large ReFmB1rth runtime/project work buffers are placed in Felucca's dedicated `.pool` region rather than exhausting 96 KiB general RAM
- [x] GitHub target build emits machine-readable XIP/RAM/pool usage and refuses release artifacts below conservative headroom thresholds
- [x] current analog-conditioned code head `50169796391de6d8a17b2762042696b6d9810dc4` passes host, WASM and real JieLi/FM-1 builds
- [x] current target image is 424,708 B XIP with 156,856 B XIP, 13,756 B general RAM and 173,132 B pool headroom

**M5 target integration still open:**

- [ ] expose the ReFmB1rth project save/load hooks through the physical FM-1 UI
- [ ] finish dedicated FM-1 stock-303 labels/layout and secondary EXT/MOD page instead of reusing generic Felucca card labels
- [ ] transfer the browser's improved ACID note-programming workflow to a compact FM-1 step/note editor
- [ ] implement the physical FM-1 mixer/song/pattern/drum UI
- [ ] perform controlled listening/reference-render A/B calibration against Open303/js303 for oscillator level, resonance, accent, slide and VCA contour
- [ ] decide from A/B evidence whether the low-frequency all-pass/notch stages seen in Open303/js303 materially improve the model before adding them
- [ ] measure worst-case CPU/audio-underrun cost of two simultaneous analog-conditioned 2x ACID engines plus both dense drum machines on target hardware
- [ ] listen/measure and explicitly approve the small FM-1 sample candidate set; full browser sample kits must not be copied automatically into target flash
- [ ] embed only approved target sample candidates and re-run XIP/CPU/audio-underrun gates
- [ ] move WASM DSP production fully off main-thread scheduling (the current AudioWorklet sink is a buffered timing prototype, not the final Worker/Worklet architecture)
- [ ] build the full React editor around the validated WASM ABI
- [ ] add an explicit stack-watermark/runtime stack gate in addition to the static RAM/pool gate
- [ ] expand FX with master filter resonance/mode and optional reverb only after CPU budget is measured

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
- [ ] worst-case CPU/audio-underrun gates pass, including the analog-conditioned 2x ACID ladder path
- [ ] runtime stack watermark/headroom is measured under worst-case UI + audio + MIDI activity
- [ ] return-to-stock and FM-1-transporter recovery preparation is confirmed for the test unit
- [ ] a controlled physical FM-1 beta installation is performed successfully
- [ ] post-install USB MIDI, TRS MIDI, USB audio, storage and recovery are verified on hardware

A successful GitHub build proves reproducibility and target-toolchain compatibility; it does **not** by itself make the artifact safe to flash. A physical flash is deliberately never performed by CI. There is no zero-brick guarantee.
