#!/usr/bin/env python3
import pathlib
import re
import unittest

from tools.physical_validation_report import REQUIRED_CHECKS

ROOT = pathlib.Path(__file__).resolve().parents[1]


class PhysicalValidationBrowserTests(unittest.TestCase):
    def test_validation_page_and_runtime_evidence_are_wired(self):
        html = (ROOT / "web" / "app" / "index.html").read_text()
        runtime = (ROOT / "web" / "app" / "runtime_diagnostics.js").read_text()
        validation = (ROOT / "web" / "app" / "validation.js").read_text()

        self.assertIn('data-tab="validation"', html)
        self.assertIn('id="validation"', html)
        self.assertIn('./validation.js', html)
        self.assertIn("refm-runtime-diagnostic", runtime)
        self.assertIn("new CustomEvent('refm-runtime-diagnostic'", runtime)
        self.assertIn("window.addEventListener('refm-runtime-diagnostic'", validation)
        self.assertIn("validationAssignFactory", validation)
        self.assertIn("validationAssignStress", validation)

    def test_report_is_bound_to_exact_firmware_channel_target(self):
        validation = (ROOT / "web" / "app" / "validation.js").read_text()
        self.assertIn("firmware-channel/latest.json", validation)
        self.assertIn("state.target?.commit&&state.target.commit!==m.commit", validation)
        self.assertIn("state=emptyState()", validation)
        self.assertIn("tag:m.tag,commit:m.commit,asset:m.asset,sha256:m.sha256", validation)
        self.assertIn("Firmware SHA-256", validation)

    def test_release_ready_requires_every_physical_check_and_both_runtime_gates(self):
        validation = (ROOT / "web" / "app" / "validation.js").read_text()
        self.assertIn("CHECKS.every(([id])=>state.checks[id]?.status==='pass')", validation)
        self.assertIn("state.runtime.factory?.report?.pass===true", validation)
        self.assertIn("state.runtime.stress?.report?.pass===true", validation)
        self.assertIn("'rollback','Recovery'", validation)
        self.assertIn("'repeat_cycle','Recovery'", validation)
        self.assertIn("'usb_midi','I/O'", validation)
        self.assertIn("'trs_midi','I/O'", validation)
        self.assertIn("'usb_audio','I/O'", validation)

    def test_browser_and_offline_required_check_sets_match(self):
        validation = (ROOT / "web" / "app" / "validation.js").read_text()
        ids = tuple(re.findall(r"^\s*\['([a-z0-9_]+)','", validation, flags=re.MULTILINE))
        self.assertEqual(ids, REQUIRED_CHECKS)

    def test_validation_evidence_is_persistent_and_exportable(self):
        validation = (ROOT / "web" / "app" / "validation.js").read_text()
        self.assertIn("localStorage.setItem(STORAGE_KEY", validation)
        self.assertIn("localStorage.getItem(STORAGE_KEY)", validation)
        self.assertIn("EXPORT JSON", (ROOT / "web" / "app" / "index.html").read_text())
        self.assertIn("EXPORT MARKDOWN", (ROOT / "web" / "app" / "index.html").read_text())
        self.assertIn("JSON.stringify(reportObject(),null,2)", validation)
        self.assertIn("function markdown()", validation)


if __name__ == "__main__":
    unittest.main()
