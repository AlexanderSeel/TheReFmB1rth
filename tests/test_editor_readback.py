#!/usr/bin/env python3
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class EditorReadbackContractTests(unittest.TestCase):
    def test_firmware_contract_and_audio_context_delivery_are_wired(self):
        overlay = (ROOT / "tools" / "apply_felucca_overlay.py").read_text()
        readback = (ROOT / "firmware" / "integration" / "refm_editor_readback.c").read_text()
        self.assertIn('integration/refm_editor_readback.c', overlay)
        self.assertIn('refm_felucca_request_state_readback', overlay)
        self.assertIn('refm_felucca_emit_state_readback_if_pending', overlay)
        self.assertIn('refm_platform_midi_out_event', overlay)
        self.assertIn('cc == 107u', overlay)
        self.assertIn('emit_cc(107u, 1u)', readback)
        self.assertIn('emit_cc(107u, 127u)', readback)
        self.assertIn('static volatile uint8_t g_readback_pending', readback)

    def test_browser_contract_matches_firmware(self):
        browser = (ROOT / "web" / "app" / "readback.js").read_text()
        app = (ROOT / "web" / "app" / "app.js").read_text()
        self.assertIn('[0xBF,107,127]', browser)
        self.assertIn('machine:108', browser)
        self.assertIn('project:124', browser)
        self.assertIn("import { installReadback } from './readback.js';", app)
        self.assertIn('installReadback({state,refreshSequencer,ensureMidi,setMidiStatus});', app)


if __name__ == '__main__':
    unittest.main()
