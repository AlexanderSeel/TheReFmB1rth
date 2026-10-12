#!/usr/bin/env python3
"""Validate an exported FM-1 physical-validation JSON report."""
from __future__ import annotations

import argparse
import json
import pathlib
import sys

EXPECTED_SCHEMA = 1
RUNTIME_SLOTS = ("factory", "stress")
REQUIRED_CHECKS = (
    "install",
    "home", "acid", "step", "drum", "mixer", "fx", "song", "project",
    "encoders", "keys", "transport",
    "step_edit", "drum_edit",
    "browser_sync", "project_backup", "project_restore",
    "usb_midi", "trs_midi", "usb_audio",
    "physical_save", "corrupt_recovery",
    "rollback", "repeat_cycle",
)


def fail(msg: str) -> None:
    raise ValueError(msg)


def validate(report: dict, expected_tag: str | None = None, expected_commit: str | None = None,
             expected_sha256: str | None = None) -> dict:
    if report.get("schema") != EXPECTED_SCHEMA:
        fail(f"unsupported validation schema: {report.get('schema')!r}")

    target = report.get("target") or {}
    for key in ("tag", "commit", "sha256"):
        if not target.get(key):
            fail(f"missing target.{key}")

    if expected_tag and target["tag"] != expected_tag:
        fail(f"target tag mismatch: {target['tag']} != {expected_tag}")
    if expected_commit and target["commit"] != expected_commit:
        fail(f"target commit mismatch: {target['commit']} != {expected_commit}")
    if expected_sha256 and target["sha256"].lower() != expected_sha256.lower():
        fail(f"target SHA-256 mismatch: {target['sha256']} != {expected_sha256}")

    checks = report.get("checks") or {}
    if not checks:
        fail("validation report contains no physical checks")
    missing_checks = sorted(set(REQUIRED_CHECKS) - set(checks))
    unknown_checks = sorted(set(checks) - set(REQUIRED_CHECKS))
    if missing_checks:
        fail(f"validation report is incomplete; missing checks: {', '.join(missing_checks)}")

    failed = sorted(k for k in REQUIRED_CHECKS if (checks.get(k) or {}).get("status") == "fail")
    pending = sorted(k for k in REQUIRED_CHECKS if (checks.get(k) or {}).get("status") == "pending")
    not_pass = sorted(k for k in REQUIRED_CHECKS if (checks.get(k) or {}).get("status") != "pass")

    runtime = report.get("runtime") or {}
    runtime_failures: list[str] = []
    runtime_summary: dict[str, dict] = {}
    for slot in RUNTIME_SLOTS:
        evidence = runtime.get(slot)
        if not evidence:
            runtime_failures.append(f"{slot}: missing")
            continue
        gate = evidence.get("report") or {}
        snapshot = evidence.get("snapshot") or {}
        runtime_summary[slot] = {
            "pass": gate.get("pass") is True,
            "sustained_cpu_pct": gate.get("sustained_cpu_pct"),
            "peak_render_pct": gate.get("peak_render_pct"),
            "late": snapshot.get("late"),
            "ustack_free": snapshot.get("ustack_free"),
            "sstack_free": snapshot.get("sstack_free"),
        }
        if gate.get("pass") is not True:
            reasons = gate.get("failures") or ["runtime gate did not pass"]
            runtime_failures.append(f"{slot}: {'; '.join(map(str, reasons))}")

    ready = not not_pass and not runtime_failures
    summary = {
        "pass": ready,
        "target": {k: target.get(k) for k in ("tag", "commit", "sha256", "asset", "bytes")},
        "checks_total": len(REQUIRED_CHECKS),
        "checks_failed": failed,
        "checks_pending": pending,
        "checks_not_pass": not_pass,
        "checks_unknown": unknown_checks,
        "runtime": runtime_summary,
        "runtime_failures": runtime_failures,
    }
    return summary


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("report", type=pathlib.Path)
    p.add_argument("--expect-tag")
    p.add_argument("--expect-commit")
    p.add_argument("--expect-sha256")
    p.add_argument("--output", type=pathlib.Path)
    args = p.parse_args(argv)

    try:
        data = json.loads(args.report.read_text(encoding="utf-8"))
        summary = validate(data, args.expect_tag, args.expect_commit, args.expect_sha256)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(f"physical validation: FAIL: {exc}", file=sys.stderr)
        return 2

    text = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    print(text, end="")
    if not summary["pass"]:
        print("physical validation: FAIL", file=sys.stderr)
        return 1
    print("physical validation: PASS", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
