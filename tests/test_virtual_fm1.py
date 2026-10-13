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

    def test_editor_virtual_route_is_explicit_and_transport_is_single_owner(self):
        bridge = (ROOT / 'web/app/virtual_editor_bridge.js').read_text()
        transport = (ROOT / 'web/app/transport.js').read_text()
        self.assertIn('window.refmVirtual?.routeActive', bridge)
        self.assertIn('window.refmVirtual?.routeActive', transport)
        self.assertIn('transport.js owns realtime start/continue/stop exactly once', bridge)
        self.assertNotIn("send([c==='start'?0xFA", bridge)
        self.assertIn('VIRTUAL FM-1', transport)

    def test_panel_uses_reference_fm1_geometry_and_27_key_index_map(self):
        js = (ROOT / 'web/app/virtual_fm1.js').read_text()
        css = (ROOT / 'web/app/virtual_fm1.css').read_text()
        self.assertIn('const WHITE_K=[0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26]', js)
        self.assertIn('const BLACK_K=[1,3,5,8,10,13,15,17,20,22,25]', js)
        self.assertIn('const BLACK_AFTER=[0,1,2,4,5,7,8,9,11,12,14]', js)
        self.assertIn("knob(105,100,24,'MASTER'", js)
        self.assertIn("knob(215,100,24,'SELECT'", js)
        self.assertIn('x:568,y:160,width:400,height:140', js)
        self.assertIn('left:31.8%', css)
        self.assertIn('top:9.7%', css)
        self.assertIn('width:20%', css)
        self.assertIn('height:31.25%', css)

    def test_virtual_lcd_tracks_target_beta_renderer_contract(self):
        js = (ROOT / 'web/app/virtual_fm1.js').read_text()
        for marker in (
            "card(3,61,'TRACK'", "card(62,61,'PATTERN'", "card(121,61,'STEP'",
            "card(3,61,'CUTOFF'", "steps(selStep,playing?playStep:-1)",
            "card(3,139,'LANE'", "card(3,61,'LEVEL'", "card(3,61,'DRIVE'",
            "footer('OCT- LOAD   OCT+ SAVE   HOME EXIT')",
        ):
            self.assertIn(marker, js)
        self.assertIn("if(value(0)===7)restoreLocal();else encoder(ENC.ALGORITHM,-1)", js)
        self.assertIn("if(value(0)===7)saveLocal();else encoder(ENC.ALGORITHM,1)", js)

    def test_rb240_skin_is_shared_between_hardware_and_virtual(self):
        js = (ROOT / 'web/app/virtual_fm1.js').read_text()
        css = (ROOT / 'web/app/virtual_fm1.css').read_text()
        skin = (ROOT / 'tools/apply_rb240_skin.py').read_text()
        overlay = (ROOT / 'tools/patch_fm1_rebirth_ui.py').read_text()
        for marker in ("led:'#250708'", "red:'#ff4a3f'", "amber:'#ffb326'", 'function bevel(', 'function ledBox('):
            self.assertIn(marker, js)
        for marker in ('--rb240-metal', '--rb240-red', '--rb240-amber', '--rb240-cyan'):
            self.assertIn(marker, css)
        for marker in ('RB240_PANEL', 'RB240_LED_BG', 'RB240_RED', 'RB240_AMBER', 'refm_bevel'):
            self.assertIn(marker, skin)
        self.assertIn('apply_rb240_skin.py', overlay)
        self.assertIn('rb240-metal-panel-v1', overlay)

    def test_virtual_audio_uses_buffered_worklet_not_deprecated_script_processor(self):
        js = (ROOT / 'web/app/virtual_fm1.js').read_text()
        client = (ROOT / 'web/emu/worklet_client.js').read_text()
        self.assertIn('ReFmWorkletClient', js)
        self.assertIn('refm-worklet-stats', js)
        self.assertNotIn('createScriptProcessor', js)
        self.assertIn("new URL('./refm_audio_worklet.js', import.meta.url)", client)
        self.assertIn("new AudioWorkletNode(this.ctx, 'refm-audio'", client)
        self.assertIn('underflows', client)


if __name__ == '__main__':
    unittest.main()
