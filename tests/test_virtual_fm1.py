#!/usr/bin/env python3
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def compact(text: str) -> str:
    return re.sub(r'\s+', '', text)


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
        packed = compact(src)
        self.assertIn('#include"../../firmware/proto/ui240.h"', packed)
        self.assertIn('refm_wasm_panel_encoder', src)
        self.assertIn('refm_wasm_panel_action', src)
        self.assertIn('refm_wasm_panel_key', src)
        self.assertIn('ui240_snapshot(&panel_ui,&vm.groovebox,&panel_view)', packed)
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

    def test_virtual_renderer_v2_matches_pattern_first_screen_language(self):
        shell = (ROOT / 'web/app/virtual_fm1.js').read_text()
        entry = (ROOT / 'web/app/rb240_renderer.js').read_text()
        renderer = (ROOT / 'web/app/rb240_renderer_v2.js').read_text()
        self.assertIn("import {drawRB240} from './rb240_renderer.js'", shell)
        self.assertIn("rb240_renderer_v2.js", entry)
        for marker in (
            "'SEQ EDIT'", "'TB-303 STYLE STEP EDIT'", "'TR-808 STEP PROGRAMMER'", "'TR-909 STEP PROGRAMMER'",
            "'PCF'", "'DELAY'", "'DIST'", "'COMP'", "'FLAM'", "'MIXER'",
            "'PATTERN / SONG'", 'function knob(', 'function meter(', 'function acidStrip(', 'function drumGrid('
        ):
            self.assertIn(marker, renderer)
        self.assertIn('Array.from({length:16}', renderer)
        self.assertIn('drumSteps=packed.map', renderer)
        self.assertIn('window.refmRB240State=s', renderer)
        self.assertIn('focus=!!v(34)', renderer)
        self.assertIn('focusRegion=v(35)&7', renderer)

    def test_beta3_panel_state_is_shared_by_target_and_wasm(self):
        ui_h = (ROOT / 'firmware/proto/ui240.h').read_text()
        ui_c = (ROOT / 'firmware/proto/ui240.c').read_text()
        target = (ROOT / 'firmware/integration/refm_felucca_bridge.c').read_text()
        wasm = (ROOT / 'web/emu/refm_wasm.c').read_text()
        wasm_packed = compact(wasm)
        for marker in ('UI240_FX_PCF','UI240_FX_DELAY','UI240_FX_DIST','UI240_FX_COMP','fx_section'):
            self.assertIn(marker, ui_h)
        for marker in ('UI240_FX_PCF','UI240_FX_DELAY','UI240_FX_DIST','UI240_FX_SECTION_COUNT','fx_section'):
            self.assertIn(marker, ui_c)
        self.assertIn('ui240_cycle_fx_section(&g_ui, d)', target)
        self.assertIn('ui240_cycle_fx_section(&panel_ui,d)', wasm_packed)
        for marker in ('case 29:', 'case 30:', 'case 31:', 'case 32:'):
            self.assertIn(marker, target)
        for marker in ('case29u:returnpanel_view.acid_cutoff_pct;', 'case30u:returnpanel_view.acid_resonance_pct;', 'case31u:returnpanel_view.acid_env_pct;', 'case32u:returnpanel_view.acid_decay_pct;'):
            self.assertIn(marker, wasm_packed)
        for marker in ('acid_cutoff_pct','acid_resonance_pct','acid_env_pct','acid_decay_pct'):
            self.assertIn(marker, ui_h)
            self.assertIn(marker, ui_c)

    def test_beta3_physical_overlay_applies_same_seq_drum_fx_contract(self):
        base = (ROOT / 'tools/apply_rb240_skin.py').read_text()
        beta3 = (ROOT / 'tools/apply_rb240_beta3_layout.py').read_text()
        flam = (ROOT / 'tools/apply_rb240_beta3_flam.py').read_text()
        overlay = (ROOT / 'tools/patch_fm1_rebirth_ui.py').read_text()
        for marker in ('RB240_PANEL','RB240_LED_BG','RB240_AMBER','RB240_CYAN','refm_knob','refm_button'):
            self.assertIn(marker, base)
        for marker in ('SEQ EDIT','TR-808 RHYTHM','TR-909 RHYTHM','PCF','DELAY','DIST','COMP'):
            self.assertIn(marker, beta3)
        for marker in ('909-flam=real','K3-toggle,K4-width','machine == 3u && p2 != 0u'):
            self.assertIn(marker, flam)
        self.assertIn('apply_rb240_beta3_layout.py', overlay)
        self.assertIn('apply_rb240_beta3_flam.py', overlay)
        self.assertIn('rb240-beta3-approved', overlay)

    def test_909_flam_is_real_persisted_engine_state(self):
        renderer = (ROOT / 'web/app/rb240_renderer_v2.js').read_text()
        groove = (ROOT / 'firmware/proto/groovebox.c').read_text()
        bank = (ROOT / 'firmware/proto/pattern_bank.h').read_text()
        self.assertIn("'FLAM'", renderer)
        self.assertIn('drum_909_flams', groove)
        self.assertIn('flam_pending', groove)
        self.assertIn('0xABu', groove)
        self.assertIn('drum_909_flams', bank)

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
