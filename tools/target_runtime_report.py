#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Evaluate one physical FM-1 runtime diagnostic snapshot.

The snapshot is expected to contain Felucca/ReFm `felucca_dbg` fields.  This
combines the audio deadline gate with conservative user/supervisor stack
headroom checks so one JSON capture can be attached to a hardware validation.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from target_cpu_report import evaluate as evaluate_cpu

DEFAULT_MIN_USTACK_FREE = 4096
DEFAULT_MIN_SSTACK_FREE = 1024


def evaluate_runtime(
    snapshot: dict[str, Any],
    half_frames: int,
    max_sustained_pct: float = 75.0,
    max_peak_pct: float = 82.0,
    min_ustack_free: int = DEFAULT_MIN_USTACK_FREE,
    min_sstack_free: int = DEFAULT_MIN_SSTACK_FREE,
) -> dict[str, Any]:
    report = evaluate_cpu(snapshot, half_frames, max_sustained_pct, max_peak_pct)
    failures = list(report["failures"])

    required = ("ustack_used", "ustack_free", "sstack_used", "sstack_free")
    missing = [name for name in required if name not in snapshot]
    if missing:
        failures.append("missing runtime stack fields: " + ", ".join(missing))
        u_used = u_free = s_used = s_free = 0
    else:
        u_used = int(snapshot["ustack_used"])
        u_free = int(snapshot["ustack_free"])
        s_used = int(snapshot["sstack_used"])
        s_free = int(snapshot["sstack_free"])
        for name, value in (
            ("ustack_used", u_used),
            ("ustack_free", u_free),
            ("sstack_used", s_used),
            ("sstack_free", s_free),
        ):
            if value < 0:
                failures.append(f"{name} must not be negative")
        if u_free < min_ustack_free:
            failures.append(
                f"user-stack free headroom {u_free} B is below {min_ustack_free} B gate"
            )
        if s_free < min_sstack_free:
            failures.append(
                f"supervisor-stack free headroom {s_free} B is below {min_sstack_free} B gate"
            )

    report["pass"] = not failures
    report["failures"] = failures
    report["stack"] = {
        "user": {"used_bytes": u_used, "free_bytes": u_free},
        "supervisor": {"used_bytes": s_used, "free_bytes": s_free},
        "limits": {
            "min_user_free_bytes": min_ustack_free,
            "min_supervisor_free_bytes": min_sstack_free,
        },
        "note": (
            "user-stack usage is conservative because the active early-boot frame is left unpainted; "
            "supervisor-stack watermark covers the complete configured supervisor stack"
        ),
    }
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--half-frames", type=int, required=True)
    parser.add_argument("--max-sustained-pct", type=float, default=75.0)
    parser.add_argument("--max-peak-pct", type=float, default=82.0)
    parser.add_argument("--min-ustack-free", type=int, default=DEFAULT_MIN_USTACK_FREE)
    parser.add_argument("--min-sstack-free", type=int, default=DEFAULT_MIN_SSTACK_FREE)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    snapshot = json.loads(args.snapshot.read_text(encoding="utf-8"))
    if not isinstance(snapshot, dict):
        raise SystemExit("snapshot must be a JSON object")

    report = evaluate_runtime(
        snapshot,
        args.half_frames,
        args.max_sustained_pct,
        args.max_peak_pct,
        args.min_ustack_free,
        args.min_sstack_free,
    )
    stack = report["stack"]
    print(
        f"FM-1 runtime: CPU {report['sustained_cpu_pct']:.1f}% sustained / "
        f"{report['peak_render_pct']:.1f}% peak, late={report['late']} | "
        f"U-stack free={stack['user']['free_bytes']} B | "
        f"S-stack free={stack['supervisor']['free_bytes']} B | "
        f"{'PASS' if report['pass'] else 'FAIL'}"
    )
    for failure in report["failures"]:
        print(f"  - {failure}")

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
