#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Patch the built virtual FM-1 panel to use the shared performance ABI."""
from __future__ import annotations
import sys
from pathlib import Path


def rep(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise SystemExit(f"virtual performance anchor mismatch: {old[:80]!r}")
    return text.replace(old, new, 1)


def main() -> int:
    path = Path(sys.argv[1])
    s = path.read_text()
    s = rep(
        s,
        "const ACTION={HOME:0,ENV:1,LFO:2,SEQ:3,ARP:4,GLO:5,FX:6,SEL:7,SAVE:8,PLAY:9,EDIT:10,REC:11};\n",
        "const ACTION={HOME:0,ENV:1,LFO:2,SEQ:3,ARP:4,GLO:5,FX:6,SEL:7,SAVE:8,PLAY:9,EDIT:10,REC:11};\n"
        "const MOD={NONE:0,SEQ:1,DRUM:2,MIXER:3,FX:4,PATTERN:5,EDIT:6,LIVE:7,FOCUS:8};\n"
        "const BUTTON_MOD={SEQ:1,ARP:2,GLO:3,FX:4,SAVE:5,EDIT:6,REC:7,SEL:8};\n"
        "let heldModifierButton=null,modifierUsed=false;\n",
    )
    s = rep(
        s,
        "function value(i){return ready?mod._refm_wasm_panel_value(i)>>>0:0;}\n"
        "function action(id){if(ready){mod._refm_wasm_panel_action(id);drawScreen();}}\n"
        "function encoder(id,d){if(ready&&d){mod._refm_wasm_panel_encoder(id,d);drawScreen();}}\n"
        "function key(panelIndex,on){if(ready){mod._refm_wasm_panel_key(MIDI_BASE+panelIndex,on?1:0);drawScreen();}}\n",
        "function value(i){if(!ready)return 0;const f=mod._refm_wasm_panel_performance_value||mod._refm_wasm_panel_value;return f(i)>>>0;}\n"
        "function modifier(id){if(ready&&mod._refm_wasm_panel_modifier)mod._refm_wasm_panel_modifier(id);}\n"
        "function focusExit(){if(ready&&mod._refm_wasm_panel_focus_exit)mod._refm_wasm_panel_focus_exit();}\n"
        "function action(id){if(ready){const f=mod._refm_wasm_panel_performance_action||mod._refm_wasm_panel_action;f(id);drawScreen();}}\n"
        "function encoder(id,d){if(ready&&d){if(heldModifierButton)modifierUsed=true;const f=mod._refm_wasm_panel_performance_encoder||mod._refm_wasm_panel_encoder;f(id,d);drawScreen();}}\n"
        "function key(panelIndex,on){if(ready){if(on&&heldModifierButton)modifierUsed=true;if(mod._refm_wasm_panel_key_event)mod._refm_wasm_panel_key_event(panelIndex,MIDI_BASE+panelIndex,on?1:0);else mod._refm_wasm_panel_key(MIDI_BASE+panelIndex,on?1:0);drawScreen();}}\n",
    )
    old = "const fire=()=>{if(name==='OCT-'){if(value(0)===7)restoreLocal();else encoder(ENC.ALGORITHM,-1);}else if(name==='OCT+'){if(value(0)===7)saveLocal();else encoder(ENC.ALGORITHM,1);}else action(ACTION[name]);};g.addEventListener('pointerdown',()=>{g.classList.add('held');fire();});g.addEventListener('pointerup',()=>g.classList.remove('held'));g.addEventListener('pointercancel',()=>g.classList.remove('held'));return g;}"
    new = "const fire=()=>{if(name==='OCT-'){if(value(0)===7)restoreLocal();else encoder(ENC.ALGORITHM,-1);}else if(name==='OCT+'){if(value(0)===7)saveLocal();else encoder(ENC.ALGORITHM,1);}else action(ACTION[name]);};const down=e=>{g.classList.add('held');if(BUTTON_MOD[name]!==undefined){heldModifierButton=name;modifierUsed=false;modifier(BUTTON_MOD[name]);if(g.setPointerCapture)g.setPointerCapture(e.pointerId);return;}fire();};const finish=(e,cancel=false)=>{g.classList.remove('held');if(BUTTON_MOD[name]!==undefined&&heldModifierButton===name){const used=modifierUsed,tap=!cancel&&!used;modifier(MOD.NONE);heldModifierButton=null;modifierUsed=false;if(name==='SEL'&&!used)focusExit();if(tap)fire();drawScreen();}if(e&&g.hasPointerCapture&&g.hasPointerCapture(e.pointerId))g.releasePointerCapture(e.pointerId);};g.addEventListener('pointerdown',down);g.addEventListener('pointerup',e=>finish(e,false));g.addEventListener('pointercancel',e=>finish(e,true));return g;}"
    s = rep(s, old, new)
    s = rep(
        s,
        "window.addEventListener('blur',()=>{for(const k of held.values())key(k,0);held.clear();});",
        "window.addEventListener('blur',()=>{for(const k of held.values())key(k,0);held.clear();modifier(MOD.NONE);heldModifierButton=null;modifierUsed=false;});",
    )
    path.write_text(s)
    print("virtual FM-1 tap/hold keyboard-modifier/focus parity patch applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
