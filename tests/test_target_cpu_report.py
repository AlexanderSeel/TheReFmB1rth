import unittest

from tools.target_cpu_report import evaluate


class TargetCpuReportTests(unittest.TestCase):
    def test_clean_snapshot_passes_with_margin(self):
        report = evaluate(
            {"halves": 5000, "late": 0, "cpu_q8": 160, "last_us": 700, "max_us": 850},
            half_frames=64,
            max_sustained_pct=75.0,
            max_peak_pct=82.0,
        )
        self.assertTrue(report["pass"])
        self.assertEqual(report["late"], 0)
        self.assertLess(report["sustained_cpu_pct"], 75.0)

    def test_any_dma_late_is_failure(self):
        report = evaluate(
            {"halves": 5000, "late": 1, "cpu_q8": 100, "last_us": 400, "max_us": 500},
            half_frames=64,
            max_sustained_pct=75.0,
            max_peak_pct=82.0,
        )
        self.assertFalse(report["pass"])
        self.assertTrue(any("deadline" in item for item in report["failures"]))

    def test_sustained_and_peak_limits_are_independent(self):
        deadline_us = 64 * 1_000_000.0 / 44100.0
        report = evaluate(
            {
                "halves": 2000,
                "late": 0,
                "cpu_q8": 210,
                "last_us": int(deadline_us * 0.5),
                "max_us": int(deadline_us * 0.9),
            },
            half_frames=64,
            max_sustained_pct=75.0,
            max_peak_pct=82.0,
        )
        self.assertFalse(report["pass"])
        self.assertGreaterEqual(len(report["failures"]), 2)

    def test_no_audio_capture_fails(self):
        report = evaluate(
            {"halves": 0, "late": 0, "cpu_q8": 0, "last_us": 0, "max_us": 0},
            half_frames=64,
            max_sustained_pct=75.0,
            max_peak_pct=82.0,
        )
        self.assertFalse(report["pass"])


if __name__ == "__main__":
    unittest.main()
