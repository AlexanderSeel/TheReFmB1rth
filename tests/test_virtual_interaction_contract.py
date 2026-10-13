# SPDX-License-Identifier: GPL-3.0-only
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "web" / "app" / "virtual_fm1.js"
PATCHER = ROOT / "tools" / "patch_virtual_fm1_performance.py"


class VirtualInteractionContractTests(unittest.TestCase):
    def setUp(self):
        self.source = SOURCE.read_text(encoding="utf-8")

    def test_performance_abi_is_canonical_source(self):
        required = (
            "const MOD={NONE:0,SEQ:1,DRUM:2,MIXER:3,FX:4,PATTERN:5,EDIT:6,LIVE:7,FOCUS:8}",
            "const BUTTON_MOD={SEQ:1,ARP:2,GLO:3,FX:4,SAVE:5,EDIT:6,REC:7,SEL:8}",
            "_refm_wasm_panel_performance_value",
            "_refm_wasm_panel_performance_action",
            "_refm_wasm_panel_performance_encoder",
            "_refm_wasm_panel_key_event",
            "_refm_wasm_panel_modifier",
        )
        for marker in required:
            self.assertIn(marker, self.source)

    def test_modifier_primary_action_is_deferred_until_release(self):
        self.assertIn("heldModifierButtons.set(name,state);syncModifier()", self.source)
        self.assertNotIn("g.classList.add('held');fire();", self.source)
        self.assertIn("tap=wasPrimary&&!cancel&&!used", self.source)
        self.assertIn("if(tap)fire()", self.source)

    def test_multiple_modifier_buttons_are_tracked_and_prioritized(self):
        self.assertIn("heldModifierButtons=new Map()", self.source)
        self.assertIn("const MOD_PRIORITY=['SEQ','ARP','GLO','FX','SAVE','EDIT','REC','SEL']", self.source)
        self.assertIn("function activeModifierButton()", self.source)
        self.assertIn("function syncModifier()", self.source)
        self.assertIn("heldModifierButtons.delete(name)", self.source)
        self.assertGreaterEqual(self.source.count("syncModifier()"), 3)

    def test_second_button_consumes_primary_tap(self):
        self.assertIn("else if(primaryModifierButton!==name)modifierUsed=true", self.source)
        self.assertIn("consumeModifier();fire()", self.source)
        self.assertIn("else if(m.action!==undefined){consumeModifier();action(m.action);}", self.source)

    def test_long_hold_can_latch_for_desktop_chords(self):
        self.assertIn("LONG_HOLD_MS=360", self.source)
        self.assertIn("current.latched=true", self.source)
        self.assertIn("VIRTUAL HOLD · ${name} LATCHED", self.source)
        self.assertIn("existing?.latched", self.source)
        self.assertIn("VIRTUAL HOLD RELEASED", self.source)

    def test_pc_keyboard_can_hold_panel_modifiers(self):
        self.assertIn("const PC_MOD_KEYS={KeyC:'SEQ',KeyV:'ARP',KeyB:'GLO',KeyN:'FX',KeyM:'SAVE',Comma:'EDIT',Period:'REC',Slash:'SEL'}", self.source)
        self.assertIn("function beginKeyboardModifier(name,code)", self.source)
        self.assertIn("function endKeyboardModifier(name,cancel=false)", self.source)
        self.assertIn("beginKeyboardModifier(modName,e.code)", self.source)
        self.assertIn("endKeyboardModifier(h.modifier,false)", self.source)
        self.assertIn("panelButtons.set(name,g)", self.source)

    def test_sel_tap_clears_temporary_focus_but_consumed_hold_keeps_it(self):
        self.assertIn("function focusExit()", self.source)
        self.assertIn("if(name==='SEL'&&tap)focusExit()", self.source)
        self.assertNotIn("if(name==='SEL')focusExit()", self.source)

    def test_key_or_encoder_consumes_modifier(self):
        self.assertIn("function consumeModifier()", self.source)
        self.assertIn("function encoder(id,d){if(ready&&d){consumeModifier()", self.source)
        self.assertIn("function key(panelIndex,on){if(ready){if(on)consumeModifier()", self.source)

    def test_blur_releases_all_virtual_modifiers(self):
        self.assertIn("function clearModifiers()", self.source)
        self.assertIn("heldModifierButtons.clear()", self.source)
        self.assertIn("held.clear();clearModifiers();", self.source)

    def test_compatibility_patcher_is_idempotent_for_current_source(self):
        with tempfile.TemporaryDirectory() as td:
            copy = Path(td) / "virtual_fm1.js"
            copy.write_text(self.source, encoding="utf-8")
            before = copy.read_bytes()
            subprocess.run(["python3", str(PATCHER), str(copy)], check=True, cwd=ROOT)
            self.assertEqual(before, copy.read_bytes())


if __name__ == "__main__":
    unittest.main()
