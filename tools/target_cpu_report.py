#!/usr/bin/env python3
"""Evaluate an FM-1/Felucca audio diagnostic snapshot.

The pinned Felucca audio ISR exposes cumulative diagnostics in `felucca_dbg`:
last_us/max_us for render time, cpu_q8 for smoothed load, and late for DMA
half-buffer deadline misses. This tool intentionally consumes a captured JSON
snapshot; it does not claim host benchmarks are equivalent to FM-1 timing.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

FS = 44100.0
DEFAULT_MAX_SUSTAINED_PCT = 75.0
DEFAULT_MAX_PEAK_PCT = 82.0


def evaluate(snapshot: dict[str, Any], half_frames: int, max_sustained_pct: float, max_peak_pct: float) -> dict[str, Any]:
    if half_frames <= 0:
        raise ValueError("half_frames must be positive")
    deadline_us = half_frames * 1_000_000.0 / FS
    cpu_q8 = int(snapshot.get("cpu_q8", 0))
    last_us = int(snapshot.get("last_us", 0))
    max_us = int(snapshot.get("max_us", 0))
    late = int(snapshot.get("late", 0))
    halves = int(snapshot.get("halves", 0))

    sustained_pct = cpu_q8 * 100.0 / 256.0
    last_pct = last_us * 100.0 / deadline_us
    peak_pct = max_us * 100.0 / deadline_us

    failures: list[str] = []
    if halves <= 0:
        failures.append("no rendered audio halves were captured")
    if late != 0:
        failures.append(f"DMA deadline misses detected: late={late}")
    if sustained_pct > max_sustained_pct:
        failures.append(
            f"sustained CPU {sustained_pct:.1f}% exceeds {max_sustained_pct:.1f}% gate"
        )
    if peak_pct > max_peak_pct:
        failures.append(f"peak audio render {peak_pct:.1f}% exceeds {max_peak_pct:.1f}% gate")

    return {
        "pass": not failures,
        "half_frames": half_frames,
        "deadline_us": round(deadline_us, 3),
        "halves": halves,
        "late": late,
        "last_us": last_us,
        "max_us": max_us,
        "sustained_cpu_pct": round(sustained_pct, 3),
        "last_render_pct": round(last_pct, 3),
        "peak_render_pct": round(peak_pct, 3),
        "limits": {
            "max_sustained_pct": max_sustained_pct,
            "max_peak_pct": max_peak_pct,
            "felucca_shed_threshold_pct": 85.0,
        },
        "failures": failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path, help="JSON object containing Felucca felucca_dbg fields")
    parser.add_argument("--half-frames", type=int, required=True, help="Felucca HALF_FRAMES used by this build")
    parser.add_argument("--max-sustained-pct", type=float, default=DEFAULT_MAX_SUSTAINED_PCT)
    parser.add_argument("--max-peak-pct", type=float, default=DEFAULT_MAX_PEAK_PCT)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    snapshot = json.loads(args.snapshot.read_text(encoding="utf-8"))
    if not isinstance(snapshot, dict):
        raise SystemExit("snapshot must be a JSON object")
    report = evaluate(snapshot, args.half_frames, args.max_sustained_pct, args.max_peak_pct)

    print(
        f"FM-1 audio CPU: sustained {report['sustained_cpu_pct']:.1f}% | "
        f"last {report['last_render_pct']:.1f}% | peak {report['peak_render_pct']:.1f}% | "
        f"late {report['late']} | {'PASS' if report['pass'] else 'FAIL'}"
    )
    for failure in report["failures"]:
        print(f"  - {failure}")

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
