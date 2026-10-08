# FM-1 target CPU / underrun profiling

A fast host benchmark is useful for regression detection, but it is **not** evidence that the FM-1 can sustain a DSP build. The physical release gate uses the timing diagnostics already maintained by the pinned Felucca audio ISR.

## What Felucca already measures

The pinned Felucca `audio.c` records these fields in `felucca_dbg`:

- `halves` — rendered DMA half-buffers;
- `last_us` — most recent audio render duration, excluding nested TIMER5 work;
- `max_us` — worst render duration seen since reset;
- `cpu_q8` — smoothed audio CPU load where `256 == 100%`;
- `late` — count of occasions where DMA advanced to another half while rendering;
- `in_audio` / `nested` — additional diagnostics useful while investigating overloads.

Felucca's own voice-shedding path is triggered after two consecutive half-buffers exceed roughly **85%** of their deadline. TheReFmB1rth must pass with meaningful margin below that emergency threshold.

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

The default gate in `tools/target_cpu_report.py` is deliberately stricter than Felucca's 85% emergency threshold:

- `late == 0` — **mandatory**; any DMA deadline miss fails;
- smoothed `cpu_q8` <= **75%**;
- `max_us` <= **82%** of one DMA-half deadline;
- the test must contain at least one rendered half-buffer.

These are beta-entry limits, not claims about the absolute silicon limit. If UI/MIDI activity produces occasional peaks above 82% without a `late` event, investigate before relaxing the threshold; do not simply increase the gate to make a build pass.

## Capturing a report

Create a JSON snapshot with the Felucca diagnostic values, for example:

```json
{
  "halves": 120000,
  "last_us": 810,
  "max_us": 1030,
  "cpu_q8": 170,
  "late": 0
}
```

Then run the checker using the build's actual `HALF_FRAMES` value:

```sh
python3 tools/target_cpu_report.py fm1-cpu.json --half-frames 64 --output build/target/cpu-report.json
```

The script calculates the half-buffer deadline from 44.1 kHz, reports sustained/last/peak percentages, and exits non-zero when the physical gate fails.

Do **not** guess `HALF_FRAMES`; use the value from the exact Felucca build being tested.

## Host regression baseline

`tests/bench_groovebox.c` exists only to detect relative DSP-cost regressions in CI. It intentionally runs an abusive two-ACID + two-drum + full-FX workload. A high `realtime_x` on an x86 GitHub runner is useful when comparing commits, but it must never be translated into an FM-1 CPU percentage.

## Release policy

A successful JieLi compile/link plus static XIP/RAM/pool checks is necessary but insufficient. Hardware beta remains blocked until this physical CPU/underrun gate and the runtime stack-watermark gate pass on the actual FM-1, with recovery preparation confirmed beforehand.
