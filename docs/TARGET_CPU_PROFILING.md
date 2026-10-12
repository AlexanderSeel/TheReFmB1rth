# FM-1 target CPU / underrun / stack profiling

A fast host benchmark is useful for regression detection, but it is **not** evidence that the FM-1 can sustain a DSP build. The physical release gate uses the timing diagnostics already maintained by the pinned Felucca audio ISR plus TheReFmB1rth's target-only runtime stack watermark.

## What Felucca already measures

The pinned Felucca `audio.c` records these fields in `felucca_dbg`:

- `halves` — rendered DMA half-buffers;
- `last_us` — most recent audio render duration, excluding nested TIMER5 work;
- `max_us` — worst render duration seen since reset;
- `cpu_q8` — smoothed audio CPU load where `256 == 100%`;
- `late` — count of occasions where DMA advanced to another half while rendering;
- `in_audio` / `nested` — additional diagnostics useful while investigating overloads.

Felucca's own voice-shedding path is triggered after two consecutive half-buffers exceed roughly **85%** of their deadline. TheReFmB1rth must pass with meaningful margin below that emergency threshold.

The currently pinned Felucca baseline defines `HALF_FRAMES` as **128**. Keep passing the value explicitly to the checker so a future upstream change cannot silently alter the calculated deadline.

## ReFm runtime stack watermark

`tools/apply_runtime_diagnostics.py` augments the pinned target build with four additional `felucca_dbg` fields:

- `ustack_used` / `ustack_free` — conservative user-stack high-water estimate and remaining painted headroom;
- `sstack_used` / `sstack_free` — supervisor/ISR-stack high-water estimate and remaining headroom.

The JieLi user stack grows downward from `_ustack_top`. During early C boot an active `fm1_cstart` frame already exists, so the patch intentionally leaves a small safety band below the current frame unpainted. That band is counted as **used**, making the user-stack result conservative rather than optimistic. The supervisor stack is painted completely before interrupts are enabled.

This instrumentation does **not** replace Felucca's hardware stack guard. The guard remains the hard fault boundary; the watermark exists to prove comfortable runtime margin during successful stress tests.

Configured pinned-stack ranges are:

- user stack: `_ustack_lo = 0x01C74100`, `_ustack_top = 0x01C7A000`;
- supervisor stack: `_sstack_lo = 0x01C7A100`, `_sstack_top = 0x01C7C000`.

The default beta gate requires at least **4096 bytes user-stack free** and **1024 bytes supervisor-stack free** after the complete stress run.

## Required stress pattern

Use the same logical load as `tests/bench_groovebox.c`:

1. ACID A: high resonance, strong env-mod, accent and continuous slides;
2. ACID B: high resonance, strong env-mod, accent and continuous slides;
3. DRUM 808: all 11 synthesized voices triggered at dense 16th-note rate;
4. DRUM 909: all 11 synthesized voices triggered at dense 16th-note rate;
5. all four mixer channels active at full level;
6. maximum practical delay sends;
7. master drive, compressor, performance low-pass and delay active;
8. exercise UI + MIDI while the pattern runs so main-loop/TIMER5 interaction is represented.

Run for at least several minutes, including transport start/stop, pattern changes and parameter edits. Reset the diagnostic counters immediately before the test if the tooling permits it so the result belongs to this run.

## Acceptance criteria

The default physical runtime gate is deliberately stricter than Felucca's 85% emergency threshold:

- `late == 0` — **mandatory**; any DMA deadline miss fails;
- smoothed `cpu_q8` <= **75%**;
- `max_us` <= **82%** of one DMA-half deadline;
- at least one rendered half-buffer;
- `ustack_free >= 4096` bytes;
- `sstack_free >= 1024` bytes.

These are beta-entry limits, not claims about the absolute silicon limit. If UI/MIDI activity produces occasional peaks above 82% without a `late` event, or stack headroom approaches the limits, investigate before relaxing the threshold; do not simply increase the gate to make a build pass.

## Capturing a report

Create a JSON snapshot with the Felucca/ReFm diagnostic values, for example:

```json
{
  "halves": 120000,
  "last_us": 810,
  "max_us": 1030,
  "cpu_q8": 170,
  "late": 0,
  "ustack_used": 7168,
  "ustack_free": 17152,
  "sstack_used": 1792,
  "sstack_free": 6144
}
```

For the complete beta gate, run:

```sh
python3 tools/target_runtime_report.py fm1-runtime.json \
  --half-frames 128 \
  --output build/target/runtime-report.json
```

The older CPU-only checker remains available when inspecting a capture from firmware that predates the stack instrumentation:

```sh
python3 tools/target_cpu_report.py fm1-cpu.json --half-frames 128 --output build/target/cpu-report.json
```

Do **not** guess `HALF_FRAMES`; use the value from the exact Felucca build being tested.

## Host regression baseline

`tests/bench_groovebox.c` exists only to detect relative DSP-cost regressions in CI. It intentionally runs an abusive two-ACID + two-drum + full-FX workload. A high `realtime_x` on an x86 GitHub runner is useful when comparing commits, but it must never be translated into an FM-1 CPU percentage.

`tests/test_target_runtime_report.py` verifies the combined physical gate logic on the host, including missing stack fields, insufficient stack headroom and audio deadline failures. It does not simulate target stack usage.

## Release policy

A successful JieLi compile/link plus static XIP/RAM/pool checks is necessary but insufficient. Hardware beta remains blocked until the physical CPU/underrun **and runtime stack-watermark** gates pass on the actual FM-1, with recovery preparation confirmed beforehand.
