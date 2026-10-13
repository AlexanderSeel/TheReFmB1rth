#!/usr/bin/env python3
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class VirtualFm1Tests(unittest.TestCase):
    def test_combined_app_exposes_virtual_panel(self):
        html = (ROOT / 'web/app/index.html').read_text()
        self.assertIn('data-tab="virtual"', html)
        self.assertIn('id="virtualDevice"', html)
        self.assertIn('id="virtualScreen" width="240" height="240"', html)
        self.assertIn('id="routeVirtual"', html)
        self.assertIn('./virtual_fm1.js', html)
        self.assertIn('./virtual_editor_bridge.js', html)
        self.assertIn('./virtual_fm1.css', html)

    def test_wasm_uses_physical_ui240_model(self):
        src = (ROOT / 'web/emu/refm_wasm.c').read_text()
        self.assertIn('#include "../../firmware/proto/ui240.h"', src)
        self.assertIn('refm_wasm_panel_encoder', src)
        self.assertIn('refm_wasm_panel_action', src)
        self.assertIn('refm_wasm_panel_key', src)
        self.assertIn('ui240_snapshot(&panel_ui, &vm.groovebox, &panel_view)', src)
        self.assertIn('seed_beta_pattern()', src)

    def test_wasm_build_bundles_combined_app_and_panel_exports(self):
        sh = (ROOT / 'web/emu/build_wasm.sh').read_text()
        for fn in ('_refm_wasm_panel_encoder','_refm_wasm_panel_action','_refm_wasm_panel_key','_refm_wasm_panel_value'):
            self.assertIn(fn, sh)
        self.assertIn('firmware/proto/ui240.c', sh)
        self.assertIn('mkdir -p "$OUT/app"', sh)
        self.assertIn('cp "$ROOT/web/app/"*.html "$ROOT/web/app/"*.js "$ROOT/web/app/"*.css "$OUT/app/"', sh)

    def test_editor_virtual_route_is_explicit(self):
        bridge = (ROOT / 'web/app/virtual_editor_bridge.js').read_text()
        transport = (ROOT / 'web/app/transport.js').read_text()
        self.assertIn('window.refmVirtual?.routeActive', bridge)
        self.assertIn('window.refmVirtual?.routeActive', transport)
        self.assertIn('VIRTUAL FM-1', transport)


if __name__ == '__main__':
    unittest.main()
