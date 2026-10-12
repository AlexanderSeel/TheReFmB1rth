#!/usr/bin/env python3
import unittest

from tools.physical_validation_report import REQUIRED_CHECKS, validate


def runtime(pass_=True):
    return {
        "snapshot": {"late": 0, "ustack_free": 8192, "sstack_free": 4096},
        "report": {
            "pass": pass_,
            "sustained_cpu_pct": 50.0,
            "peak_render_pct": 60.0,
            "failures": [] if pass_ else ["synthetic failure"],
        },
    }


def report():
    return {
        "schema": 1,
        "target": {
            "tag": "v0.1.0-beta.2",
            "commit": "2b4c537cc1a1f8c42c43ee953e14e27d7740837b",
            "sha256": "f2d2e13089a52cf279cee0bc43d9d4791ae913f0b3b889ec5b3de02b70c667b2",
            "asset": "TheReFmB1rth-experimental.fwsc",
            "bytes": 609849,
        },
        "checks": {key: {"status": "pass", "note": ""} for key in REQUIRED_CHECKS},
        "runtime": {"factory": runtime(), "stress": runtime()},
    }


class PhysicalValidationReportTests(unittest.TestCase):
    def test_healthy_report_passes_exact_target_gate(self):
        r = report()
        out = validate(
            r,
            expected_tag=r["target"]["tag"],
            expected_commit=r["target"]["commit"],
            expected_sha256=r["target"]["sha256"],
        )
        self.assertTrue(out["pass"])
        self.assertEqual(out["checks_total"], len(REQUIRED_CHECKS))
        self.assertEqual(out["checks_not_pass"], [])
        self.assertEqual(out["runtime_failures"], [])

    def test_incomplete_report_is_rejected(self):
        r = report()
        del r["checks"]["usb_audio"]
        with self.assertRaisesRegex(ValueError, "missing checks: usb_audio"):
            validate(r)

    def test_pending_or_na_physical_check_never_passes(self):
        for status in ("pending", "na", "fail"):
            with self.subTest(status=status):
                r = report()
                r["checks"]["rollback"]["status"] = status
                self.assertFalse(validate(r)["pass"])

    def test_runtime_failure_blocks_acceptance(self):
        r = report()
        r["runtime"]["stress"] = runtime(False)
        out = validate(r)
        self.assertFalse(out["pass"])
        self.assertIn("stress: synthetic failure", out["runtime_failures"])

    def test_missing_runtime_capture_blocks_acceptance(self):
        r = report()
        r["runtime"]["factory"] = None
        out = validate(r)
        self.assertFalse(out["pass"])
        self.assertIn("factory: missing", out["runtime_failures"])

    def test_exact_target_mismatch_is_rejected(self):
        r = report()
        with self.assertRaisesRegex(ValueError, "target commit mismatch"):
            validate(r, expected_commit="deadbeef")
        with self.assertRaisesRegex(ValueError, "target tag mismatch"):
            validate(r, expected_tag="v0.1.0-beta.3")
        with self.assertRaisesRegex(ValueError, "target SHA-256 mismatch"):
            validate(r, expected_sha256="00" * 32)


if __name__ == "__main__":
    unittest.main()
