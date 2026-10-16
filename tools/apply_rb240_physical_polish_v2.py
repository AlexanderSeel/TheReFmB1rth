#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Hardware screenshot correction pass for the real FM-1 240x240 panel.

Applied after full parity + target arithmetic. This version intentionally uses
anchors that exist in the final generated Felucca tree rather than assumptions
from an earlier bridge revision.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def one(text: str, old: str, new: str, label: str) -> str:
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"physical-polish-v2 anchor mismatch ({label}): expected 1, got {n}")
    return text.replace(old, new, 1)


def patch_bridge(root: Path) -> None:
    p = root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"
    t = p.read_text()
    t = one(t, "static uint8_t g_focus_region;\n",
            "static uint8_t g_focus_region;\nstatic uint8_t g_hold;\n", "hold storage")
    # Reset HOLD on an explicit engine/UI init. Static zero initialization already
    # covers the lazy boot path.
    if "    g_focus_active = 0u;\n" in t:
        t = t.replace("    g_focus_active = 0u;\n",
                      "    g_focus_active = 0u;\n    g_hold = 0u;\n", 1)
    else:
        raise SystemExit("physical-polish-v2 anchor missing (explicit init)")

    t = one(t,
'''static void cycle_machine_context(int8_t delta)
{
    ui240_machine_t next;
    if (!delta) return;
    if (g_ui.page == UI240_ACID_MAIN || g_ui.page == UI240_ACID_STEP) {
''',
'''static void cycle_machine_context(int8_t delta)
{
    ui240_machine_t next;
    if (!delta) return;
    if (g_ui.page == UI240_HOME) {
        int m = (int)g_ui.machine + (delta < 0 ? -1 : 1);
        if (m < 0) m = 3;
        if (m > 3) m = 0;
        refm_felucca_panic();
        ui240_set_machine(&g_ui, (ui240_machine_t)m);
    } else if (g_ui.page == UI240_ACID_MAIN || g_ui.page == UI240_ACID_STEP) {
''', "HOME machine select")

    t = one(t,
        "        if (encoder > REFM_ENC_K4) continue;\n"
        "        if (g_ui.page == UI240_ACID_MAIN) ui240_edit_acid_main(&g_ui, &g_refm.groovebox, encoder, d);\n",
        "        if (encoder > REFM_ENC_K4) continue;\n"
        "        if (g_ui.page == UI240_HOME) {\n"
        "            if (g_ui.machine < UI240_MACHINE_808) ui240_edit_acid_main(&g_ui, &g_refm.groovebox, encoder, d);\n"
        "            else ui240_edit_mixer(&g_ui, &g_refm.groovebox, encoder, d);\n"
        "        } else if (g_ui.page == UI240_ACID_MAIN) ui240_edit_acid_main(&g_ui, &g_refm.groovebox, encoder, d);\n",
        "HOME live knobs")

    t = one(t,
        "    if (r.consumed) return;\n"
        "    if (g_ui.machine >= UI240_MACHINE_808 || note > 127u) return;\n",
        "    if (r.consumed) return;\n"
        "    if (!on && g_hold && (g_ui.page == UI240_HOME || g_ui.page == UI240_ACID_MAIN)) return;\n"
        "    if (g_ui.machine >= UI240_MACHINE_808 || note > 127u) return;\n",
        "HOLD note release")

    t = one(t,
'''    case REFM_ACT_ALT:
        if (g_ui.page == UI240_ACID_STEP && g_ui.machine < UI240_MACHINE_808)
            ui240_toggle_acid_flag(&g_ui, g, SEQ16_SLIDE);
''',
'''    case REFM_ACT_ALT:
        if (g_ui.page == UI240_HOME || g_ui.page == UI240_ACID_MAIN) {
            g_hold ^= 1u;
            if (!g_hold) refm_felucca_panic();
        } else if (g_ui.page == UI240_ACID_STEP && g_ui.machine < UI240_MACHINE_808)
            ui240_toggle_acid_flag(&g_ui, g, SEQ16_SLIDE);
''', "REC HOLD action")

    t = one(t, "    case 35: return g_focus_region;\n    default: return 0u;\n",
            "    case 35: return g_focus_region;\n    case 36: return g_hold;\n    default: return 0u;\n",
            "HOLD readback")
    p.write_text(t)


def patch_performance(root: Path) -> None:
    p = root / "firmware" / "refm" / "proto" / "ui240_performance.h"
    t = p.read_text()
    old = '''        r.consumed = 1u;
        if (!press) return r;
        if (key < 22u) {
'''
    new = '''        r.consumed = 1u;
        if (!press) return r;
        /* HOME is performance mode: first eleven keys audition the selected
           808/909 voices, and the remaining keyboard never writes steps. */
        if (ui->page == UI240_HOME && modifier == UI240_MOD_NONE) {
            if (key < DRUM_VOICES) {
                r.drum_trigger = 1u;
                r.drum_machine = (uint8_t)(ui->machine - UI240_MACHINE_808);
                r.drum_lane = ui240_key_to_drum_lane(ui->machine, key);
                r.velocity = 108u;
            }
            return r;
        }
        if (key < 22u) {
'''
    t = one(t, old, new, "HOME drum audition")
    p.write_text(t)


def patch_leds(root: Path) -> None:
    p = root / "firmware" / "src" / "ui_input.c"
    t = p.read_text()
    helper = r'''extern uint16_t refm_felucca_ui_value(uint8_t id);
static int refm_panel_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, nd[FM1_NCOL] = {0};
    uint32_t c, k;
    uint16_t page = refm_felucca_ui_value(0u);
    uint16_t machine = refm_felucca_ui_value(1u);
    uint16_t playing = refm_felucca_ui_value(3u);
    uint16_t hold = refm_felucca_ui_value(36u);
    uint32_t active = B_HOME;
    switch (page) {
    case 0u: active = B_HOME; break;
    case 1u: active = machine == 1u ? B_LFO : B_ENV; break;
    case 2u: active = B_SEQ; break;
    case 3u: active = B_ARP; break;
    case 4u: active = B_GLO; break;
    case 5u: active = B_FX; break;
    case 6u: active = B_SCL; break;
    case 7u: active = B_SAVE; break;
    default: active = B_EDIT; break;
    }
    for (k = 0u; k < NB; ++k) led_put(nd, panel.btn[k], 1);
    for (k = 0u; k < 27u; ++k) {
        led_put(nd, 14u + k, 1);
        if ((fm1_in.notes >> k) & 1u) led_put(nl, 14u + k, 1);
    }
    led_put(nl, panel.btn[active], 1);
    if (hold) led_put(nl, panel.btn[B_REC], 1);
    if (song.octave < 0) led_put(nl, panel.btn[B_OCTDN], 1);
    else if (song.octave > 0) led_put(nl, panel.btn[B_OCTUP], 1);
    if (playing) {
        led_clear(nd, panel.btn[B_PLAY]);
        nl[LED_PLAY_GREEN >> 3] |= (uint8_t)(1u << (LED_PLAY_GREEN & 7u));
    }
    for (c = 0u; c < FM1_NCOL; ++c) {
        fm1_led[c] = nl[c]; fm1_led_dim[c] = nd[c]; fm1_led_breath[c] = 0u;
    }
    fm1_led_dim_level(0);
    return 1;
}

'''
    t = one(t, "static void ui_leds(void)\n{\n", helper + "static void ui_leds(void)\n{\n", "LED helper")
    t = one(t,
        "    if (!ready) {\n        led_pos_init();\n        ready = 1;\n    }\n",
        "    if (!ready) {\n        led_pos_init();\n        ready = 1;\n    }\n    if (refm_panel_leds()) return;\n",
        "LED handoff")
    p.write_text(t)


def patch_draw(root: Path) -> None:
    p = root / "firmware" / "src" / "ui_draw.c"
    t = p.read_text()
    keyboard = r'''static uint16_t rb8_last_hold = 0xffffu;
static void rb8_keyboard(uint8_t note)
{
    static const uint8_t white_pc[7] = {0u,2u,4u,5u,7u,9u,11u};
    static const uint8_t black_pc[5] = {1u,3u,6u,8u,10u};
    static const uint8_t xpos[5] = {12u,28u,60u,76u,92u};
    uint8_t i, oct;
    cv_begin(232, 29, RB5_BLACK);
    for (i = 0u; i < 14u; ++i) {
        uint8_t n = (uint8_t)(36u + (i / 7u) * 12u + white_pc[i % 7u]);
        cv_rect(2 + (int32_t)i * 16, 2, 14, 25, n == note ? RB5_AMBER : RB5_TEXT);
        cv_rect(15 + (int32_t)i * 16, 2, 1, 25, RB5_DARK);
    }
    for (oct = 0u; oct < 2u; ++oct)
        for (i = 0u; i < 5u; ++i) {
            uint8_t n = (uint8_t)(36u + oct * 12u + black_pc[i]);
            cv_rect(2 + (int32_t)oct * 112 + xpos[i], 2, 9, 16, n == note ? RB5_RED : RB5_BLACK);
        }
    cv_blit(4, 151);
}

'''
    t = one(t, "static void draw_refm_beta(void)\n", keyboard + "static void draw_refm_beta(void)\n", "keyboard")
    t = one(t,
        "    uint16_t song_loop = refm_felucca_ui_value(27u), song_length = refm_felucca_ui_value(28u);\n",
        "    uint16_t song_loop = refm_felucca_ui_value(27u), song_length = refm_felucca_ui_value(28u);\n"
        "    uint16_t hold = refm_felucca_ui_value(36u);\n", "HOLD draw value")
    t = one(t, "    if (force) lcd_fill(0, 0, 240, 240, RB5_BG);\n",
            "    if (!rb5.valid || page != rb5.page) lcd_fill(0, 0, 240, 240, RB5_BG);\n",
            "machine-switch clear")

    t = one(t,
        "        if (force || p0 != rb5.p0) rb5_numbox(3,62,56,42,\"CUTOFF\",(int32_t)rb5_pct(p0),RB5_RED);\n",
        "        if (force) lcd_fill(0,52,240,188,RB5_LIGHT);\n"
        "        if (force || p0 != rb5.p0) rb5_numbox(3,62,56,42,\"CUTOFF\",(int32_t)rb5_pct(p0),RB5_RED);\n",
        "ACID silver")
    t = one(t,
        "            rb5_textbox(180,109,56,39,\"EXT/MOD\",\"PANEL\",RB5_CYAN);\n"
        "        }\n        for (i = 0u; i < 16u; ++i) {\n",
        "        }\n"
        "        if (force || hold != rb8_last_hold) {\n"
        "            rb5_textbox(180,109,56,39,\"REC HOLD\",hold?\"ON\":\"OFF\",hold?RB5_GREEN:RB5_CYAN);\n"
        "            rb8_last_hold = hold;\n"
        "        }\n        for (i = 0u; i < 16u; ++i) {\n",
        "HOLD card")

    t = one(t,
        "    } else if (page == 2u) {\n        for (i = 0u; i < 16u; ++i) {\n",
        "    } else if (page == 2u) {\n        if (force) lcd_fill(0,52,240,188,RB5_LIGHT);\n        for (i = 0u; i < 16u; ++i) {\n",
        "STEP silver")
    t = one(t,
        "        if (force || p3 != rb5.p3) rb5_numbox(3,109,56,40,\"PROB\",p3,RB5_RED);\n",
        "        if (force || p3 != rb5.p3) rb5_numbox(3,109,56,40,\"PROB\",p3,RB5_RED);\n"
        "        if (force || p0 != rb5.p0) rb8_keyboard((uint8_t)p0);\n",
        "STEP keyboard draw")

    t = one(t,
        "static void rb7_drum_cell(uint8_t lane, uint8_t step, uint8_t state,\n"
        "                          uint8_t selected_lane, uint8_t selected_step,\n"
        "                          uint8_t playing, uint8_t play_step)\n",
        "static void rb7_drum_cell(uint8_t machine, uint8_t lane, uint8_t step, uint8_t state,\n"
        "                          uint8_t selected_lane, uint8_t selected_step,\n"
        "                          uint8_t playing, uint8_t play_step)\n", "drum machine skin arg")
    t = one(t,
        "    uint16_t fill = (state & 3u) == 2u ? RB5_RED : (state & 3u) == 1u ? RB5_AMBER : RB5_PANEL2;\n",
        "    uint16_t fill = (state & 3u) == 2u ? RB5_RED : (state & 3u) == 1u ? RB5_AMBER : (machine == 2u ? RB5_BLACK : RB5_PANEL2);\n",
        "808 cell skin")
    t = one(t,
        "                    rb7_drum_cell((uint8_t)i, s, state, (uint8_t)lane, (uint8_t)sel_step, (uint8_t)playing, (uint8_t)play_step);\n",
        "                    rb7_drum_cell((uint8_t)machine, (uint8_t)i, s, state, (uint8_t)lane, (uint8_t)sel_step, (uint8_t)playing, (uint8_t)play_step);\n",
        "drum cell call")
    t = one(t,
        "    } else if (page == 3u) {\n        for (i = 0u; i < 11u; ++i) {\n",
        "    } else if (page == 3u) {\n        if (force) lcd_fill(0,52,240,145,machine==2u?RB5_BLACK:RB5_PANEL2);\n        for (i = 0u; i < 11u; ++i) {\n",
        "drum background")
    t = one(t,
        "            rb5_textbox(3,202,73,34,\"LANE\",lane<11u?RB5_DRUM[lane]:\"?\",RB5_RED);\n"
        "            rb5_textbox(80,202,73,34,\"STATE\",dstate==0u?\"OFF\":dstate==1u?\"HIT\":\"ACC\",RB5_AMBER);\n"
        "            rb5_textbox(157,202,79,34,\"KIT\",machine==2u?\"808\":\"909\",RB5_CYAN);\n",
        "            rb5_textbox(3,198,73,38,\"LANE\",lane<11u?RB5_DRUM[lane]:\"?\",RB5_RED);\n"
        "            rb5_textbox(80,198,73,38,\"STATE\",dstate==0u?\"OFF\":dstate==1u?\"HIT\":\"ACC\",RB5_AMBER);\n"
        "            rb5_textbox(157,198,79,38,\"KIT\",machine==2u?\"808\":\"909\",RB5_CYAN);\n",
        "drum footer")
    p.write_text(t)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-rb240-full-parity").is_file():
        raise SystemExit("apply full RB-240 parity first")
    patch_bridge(root); patch_performance(root); patch_leds(root); patch_draw(root)
    (root / ".refm-rb240-physical-polish").write_text(
        "home=machine-select+live-knobs+drum-audition\n"
        "leds=refm-page-aware\nhold=rec-latch\nseq=keyboard-visible\n"
        "drum-footer=unclipped\n808=black\n909=grey\nacid=silver\n"
        "machine-switch=no-full-lcd-clear\n")
    print("RB-240 hardware screenshot polish v2 applied")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
