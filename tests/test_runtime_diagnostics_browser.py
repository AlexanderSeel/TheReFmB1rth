#!/usr/bin/env python3
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class BrowserRuntimeDiagnosticsTests(unittest.TestCase):
    def test_firmware_editor_command_exposes_gate_fields_read_only(self):
        header = (ROOT / "firmware" / "integration" / "refm_editor_diagnostics.h").read_text()
        patch = (ROOT / "tools" / "apply_runtime_diagnostics.py").read_text()
        self.assertIn("#define ED_REFM_DIAGNOSTICS 91u", header)
        for field in (
            "magic", "halves", "last_us", "max_us", "cpu_q8", "late",
            "ustack_used", "ustack_free", "sstack_used", "sstack_free",
        ):
            self.assertIn(f"felucca_dbg.{field}", header)
        self.assertNotIn("= 0", header.split("static int refm_diagnostics_handle", 1)[1])
        self.assertIn("refm_editor_diagnostics.h", patch)
        self.assertIn("refm_diagnostics_handle(cmd, a, na)", patch)
        self.assertIn("editor_command=91", patch)

    def test_browser_thresholds_match_target_runtime_report_defaults(self):
        browser = (ROOT / "web" / "app" / "runtime_diagnostics.js").read_text()
        runtime = (ROOT / "tools" / "target_runtime_report.py").read_text()
        cpu = (ROOT / "tools" / "target_cpu_report.py").read_text()
        html = (ROOT / "web" / "app" / "index.html").read_text()
        self.assertIn("const COMMAND=91;", browser)
        self.assertIn("const HALF_FRAMES=128;", browser)
        self.assertIn("const MAX_SUSTAINED=75;", browser)
        self.assertIn("const MAX_PEAK=82;", browser)
        self.assertIn("const MIN_USTACK_FREE=4096;", browser)
        self.assertIn("const MIN_SSTACK_FREE=1024;", browser)
        self.assertIn("DEFAULT_MIN_USTACK_FREE = 4096", runtime)
        self.assertIn("DEFAULT_MIN_SSTACK_FREE = 1024", runtime)
        self.assertIn("DEFAULT_MAX_SUSTAINED_PCT = 75.0", cpu)
        self.assertIn("DEFAULT_MAX_PEAK_PCT = 82.0", cpu)
        self.assertIn("./runtime_diagnostics.js", html)
        self.assertIn("DOWNLOAD DIAGNOSTIC JSON", browser)


if __name__ == "__main__":
    unittest.main()
