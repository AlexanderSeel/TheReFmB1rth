# Architecture

## Baseline strategy

TheReFmB1rth tracks a pinned Felucca upstream baseline rather than copying unknown factory firmware. The current pin is `3dd2b0852bc310a2c1bf00c2d443ac19202c2140` (Felucca 1.0.5).

`tools/bootstrap_felucca.sh` checks out that exact revision into `.upstream/felucca`. This is intentionally reproducible: development must not silently build against Felucca `main`.

The first development stage keeps TheReFmB1rth-specific DSP isolated under `firmware/proto/` so it can be compiled and fuzzed on a host before it is integrated with the FM-1 audio callback.

## Intended runtime graph

```text
sequencer clock
   |-- ACID 1 -> drive/filter/VCA --|
   |-- ACID 2 -> drive/filter/VCA --|--> 4-channel mixer -> send FX -> limiter -> FM-1 audio
   |-- DRUM 808 --------------------|
   `-- DRUM 909 --------------------|
```

All four machines are rendered from the same 44.1 kHz clock. The sequencer emits timestamped step events; audio DSP does not own musical state transitions.

## Acid prototype

The initial `acid303` module is not intended as final 303 emulation. It establishes four safety properties first:

1. fixed-point-only real-time path;
2. bounded output;
3. deterministic envelope/slide state;
4. no dynamic allocation.

The current filter is a deliberately stable two-pole prototype. A more characteristic nonlinear ladder/diode-ladder model will only replace it after CPU profiling against the FM-1 target budget.

## Hardware integration gates

No module under `firmware/proto/` is flashable by itself. Hardware integration happens only after:

- upstream Felucca baseline build is reproduced;
- inherited host tests pass;
- WASM emulator passes;
- new DSP host tests pass;
- linker-map XIP/RAM/pool checks pass;
- package preflight passes.

The installer must remain loader-mediated and identity-checked. Direct arbitrary flash writes are outside the normal project workflow.
