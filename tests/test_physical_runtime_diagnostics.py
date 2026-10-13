# SPDX-License-Identifier: GPL-3.0-only
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PhysicalRuntimeDiagnosticsTests(unittest.TestCase):
    def test_target_patch_tracks_refm_render_and_input_path(self):
        s = (ROOT / 'tools' / 'apply_runtime_diagnostics.py').read_text(encoding='utf-8')
        for marker in (
            'refm_blocks', 'refm_nonzero_blocks', 'g_diag_key_edges',
            'g_diag_key_consumed', 'g_diag_live_note_edges',
            'g_diag_transport_events', 'g_diag_transport_playing',
            'refm_felucca_diag_value', 'protocol=2'
        ):
            self.assertIn(marker, s)
        self.assertIn('if (out[refm_i] != 0)', s)
        self.assertIn('g_diag_key_edges++', s)
        self.assertIn('g_diag_live_note_edges++', s)
        self.assertIn('g_diag_transport_events++', s)

    def test_browser_accepts_beta3_v1_and_new_v2(self):
        s = (ROOT / 'web' / 'app' / 'runtime_diagnostics.js').read_text(encoding='utf-8')
        self.assertIn('FIELDS_V1', s)
        self.assertIn('FIELDS_V2', s)
        self.assertIn("version===1?FIELDS_V1:version===2?FIELDS_V2", s)
        for marker in ('refm_nonzero_blocks','key_edges','live_note_edges','transport_events','transport_playing'):
            self.assertIn(marker, s)
        self.assertIn('RENDERING SILENCE', s)
        self.assertIn('NON-ZERO AUDIO', s)


if __name__ == '__main__':
    unittest.main()
