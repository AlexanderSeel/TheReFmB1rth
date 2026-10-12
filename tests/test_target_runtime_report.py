#!/usr/bin/env python3
from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from target_runtime_report import evaluate_runtime  # noqa: E402


class TargetRuntimeReportTests(unittest.TestCase):
    def good_snapshot(self) -> dict[str, int]:
        return {
            "halves": 120000,
            "last_us": 1500,
            "max_us": 2100,
            "cpu_q8": 180,
            "late": 0,
            "ustack_used": 7000,
            "ustack_free": 17000,
            "sstack_used": 1800,
            "sstack_free": 6100,
        }

    def test_combined_runtime_gate_passes_healthy_snapshot(self) -> None:
        report = evaluate_runtime(self.good_snapshot(), half_frames=128)
        self.assertTrue(report["pass"])
        self.assertEqual([], report["failures"])
        self.assertEqual(17000, report["stack"]["user"]["free_bytes"])
        self.assertEqual(6100, report["stack"]["supervisor"]["free_bytes"])

    def test_runtime_gate_requires_stack_fields(self) -> None:
        snapshot = self.good_snapshot()
        del snapshot["ustack_free"]
        report = evaluate_runtime(snapshot, half_frames=128)
        self.assertFalse(report["pass"])
        self.assertTrue(any("missing runtime stack fields" in item for item in report["failures"]))

    def test_runtime_gate_rejects_low_stack_headroom(self) -> None:
        snapshot = self.good_snapshot()
        snapshot["ustack_free"] = 2048
        snapshot["sstack_free"] = 512
        report = evaluate_runtime(snapshot, half_frames=128)
        self.assertFalse(report["pass"])
        self.assertTrue(any("user-stack free headroom" in item for item in report["failures"]))
        self.assertTrue(any("supervisor-stack free headroom" in item for item in report["failures"]))

    def test_runtime_gate_keeps_audio_failures(self) -> None:
        snapshot = self.good_snapshot()
        snapshot["late"] = 1
        report = evaluate_runtime(snapshot, half_frames=128)
        self.assertFalse(report["pass"])
        self.assertTrue(any("DMA deadline misses" in item for item in report["failures"]))


if __name__ == "__main__":
    unittest.main()
