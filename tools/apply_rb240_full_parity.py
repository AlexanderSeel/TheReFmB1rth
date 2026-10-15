#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Final physical FM-1/WASM parity pass.

The 240x240 LCD cannot reproduce the desktop rack pixel-for-pixel, but it should
expose the same hierarchy and state instead of a second, reduced product model.
This pass keeps the hardware-safe dirty renderer/input architecture while making
HOME, ACID, SEQ EDIT, DRUM, MIXER, FX, SONG and PROJECT mirror the WASM rack.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(text: str, old: str, new: str, what: str) -> str:
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"full parity anchor mismatch ({what}): expected 1, got {n}")
    return text.replace(old, new, 1)


def replace_section(text: str, start: str, end: str, replacement: str, what: str) -> str:
    a = text.find(start)
    if a < 0:
        raise SystemExit(f"full parity start anchor missing ({what})")
    b = text.find(end, a + len(start))
    if b < 0:
        raise SystemExit(f"full parity end anchor missing ({what})")
    return text[:a] + replacement + text[b:]


def patch_bridge(root: Path) -> None:
    bridge = root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"
    text = bridge.read_text()

    # The pattern bank already contains all A-H factory parts. Do not overwrite A
    # with the old one-off beta test pattern after target init.
    text = text.replace(
        "        seed_beta_pattern();",
        "        groovebox_pattern_load(&g_refm.groovebox.patterns, &g_refm.groovebox, 0u);",
    )
    text = text.replace(
        "    seed_beta_pattern();",
        "    groovebox_pattern_load(&g_refm.groovebox.patterns, &g_refm.groovebox, 0u);",
    )
    text = text.replace("refm_target_init(&g_refm, 128u);", "refm_target_init(&g_refm, 136u);")
    text = text.replace("refm_target_init(&g_refm, bpm);", "(void)bpm;\n    refm_target_init(&g_refm, 136u);")

    helper = r'''
uint16_t refm_felucca_mix_value(uint8_t track, uint8_t id)
{
    const mixer_track_t *t;
    ensure_ready();
    if (track >= MIX_TRACKS) return 0u;
    t = &g_refm.groovebox.mixer.track[track];
    switch (id) {
    case 0: return (uint16_t)t->level;
    case 1: return (uint16_t)((int32_t)t->pan + 32767);
    case 2: return t->mute ? 1u : 0u;
    case 3: return t->solo ? 1u : 0u;
    case 4: return t->delay_send;
    case 5: return t->reverb_send;
    default: return 0u;
    }
}

uint16_t refm_felucca_fx_value(uint8_t id)
{
    const mixer_fx_t *m;
    ensure_ready();
    m = &g_refm.groovebox.mixer;
    switch (id) {
    case 0: return (uint16_t)m->drive;
    case 1: return (uint16_t)(32767u - (uint16_t)m->compressor_threshold);
    case 2: return (uint16_t)m->filter_cutoff;
    case 3: return (uint16_t)(((uint32_t)m->delay_feedback * 32767u) / 31000u);
    case 4: return (uint16_t)m->delay_mix;
    case 5: return m->delay_len;
    default: return 0u;
    }
}

uint8_t refm_felucca_drum_matrix_value(uint8_t machine, uint8_t lane, uint8_t step)
{
    unsigned d;
    uint16_t bit;
    uint8_t state;
    ensure_ready();
    if (machine < UI240_MACHINE_808 || machine > UI240_MACHINE_909 ||
        lane >= DRUM_VOICES || step >= SEQ16_STEPS) return 0u;
    d = (unsigned)machine - UI240_MACHINE_808;
    bit = (uint16_t)(1u << lane);
    state = (g_refm.groovebox.drum_accents[d][step] & bit) ? 2u :
            (g_refm.groovebox.drum_hits[d][step] & bit) ? 1u : 0u;
    if (machine == UI240_MACHINE_909 && (g_refm.groovebox.drum_909_flams[step] & bit))
        state |= 4u;
    return state;
}

'''
    text = replace_once(text, "uint16_t refm_felucca_ui_value(uint8_t id)\n", helper + "uint16_t refm_felucca_ui_value(uint8_t id)\n", "bridge readback helpers")
    bridge.write_text(text)


def patch_renderer(root: Path) -> None:
    draw = root / "firmware" / "src" / "ui_draw.c"
    text = draw.read_text()
    text = replace_once(
        text,
        "extern uint32_t refm_felucca_pattern_step(uint8_t step);\n",
        "extern uint32_t refm_felucca_pattern_step(uint8_t step);\n"
        "extern uint16_t refm_felucca_mix_value(uint8_t track, uint8_t id);\n"
        "extern uint16_t refm_felucca_fx_value(uint8_t id);\n"
        "extern uint8_t refm_felucca_drum_matrix_value(uint8_t machine, uint8_t lane, uint8_t step);\n",
        "renderer bridge declarations",
    )

    helpers = r'''
static uint8_t rb7_drum_cache[11u * 16u];
static uint16_t rb7_mix_cache[4u][6u];
static uint16_t rb7_fx_cache[6u];

static void rb7_pad(int32_t x, int32_t y, int32_t w, int32_t h,
                    const char *label, const char *value, uint16_t fill, uint8_t selected)
{
    cv_begin(w, h, selected ? RB5_TEXT : RB5_DARK);
    cv_rect(1, 1, w - 2, h - 2, fill);
    cv_text(4, 3, &AF_S, label, fill == RB5_AMBER || fill == RB5_RED ? RB5_INK : RB5_TEXT);
    if (value) cv_text(4, h - 15, &AF_S, value, fill == RB5_AMBER || fill == RB5_RED ? RB5_INK : RB5_TEXT);
    cv_blit(x, y);
}

static void rb7_drum_cell(uint8_t lane, uint8_t step, uint8_t state,
                          uint8_t selected_lane, uint8_t selected_step,
                          uint8_t playing, uint8_t play_step)
{
    int32_t x = 27 + (int32_t)step * 13;
    int32_t y = 55 + (int32_t)lane * 13;
    uint16_t fill = (state & 3u) == 2u ? RB5_RED : (state & 3u) == 1u ? RB5_AMBER : RB5_PANEL2;
    cv_begin(11, 11, lane == selected_lane ? RB5_TEXT : RB5_DARK);
    cv_rect(1, 1, 9, 9, fill);
    if (state & 4u) cv_rect(7, 2, 2, 2, RB5_CYAN);
    if (step == selected_step) cv_rect(1, 9, 9, 1, RB5_TEXT);
    if (playing && step == play_step) cv_rect(9, 1, 1, 8, RB5_GREEN);
    cv_blit(x, y);
}

static void rb7_mix_row(uint8_t track, uint8_t selected)
{
    static const char *const name[4] = {"ACID A","ACID B","808","909"};
    char b[12];
    uint16_t level = refm_felucca_mix_value(track, 0u);
    uint16_t pan = refm_felucca_mix_value(track, 1u);
    uint16_t mute = refm_felucca_mix_value(track, 2u);
    uint16_t solo = refm_felucca_mix_value(track, 3u);
    uint16_t dly = refm_felucca_mix_value(track, 4u);
    uint16_t rev = refm_felucca_mix_value(track, 5u);
    int32_t y = 58 + (int32_t)track * 34;
    cv_begin(234, 30, selected ? RB5_TEXT : RB5_DARK);
    cv_rect(1, 1, 232, 28, RB5_PANEL2);
    cv_text(5, 7, &AF_S, name[track], selected ? RB5_AMBER : RB5_TEXT);
    fmt_int(b, (int32_t)rb5_pct(level)); cv_text(63, 7, &AF_S, b, RB5_TEXT);
    fmt_int(b, ((int32_t)pan - 32767) * 100 / 32767); cv_text(98, 7, &AF_S, b, RB5_TEXT);
    fmt_int(b, (int32_t)dly * 100 / 127); cv_text(139, 7, &AF_S, b, RB5_TEXT);
    fmt_int(b, (int32_t)rev * 100 / 127); cv_text(176, 7, &AF_S, b, RB5_TEXT);
    cv_text(210, 7, &AF_S, solo ? "S" : mute ? "M" : "-", solo ? RB5_GREEN : mute ? RB5_RED : RB5_DIM);
    cv_blit(3, y);
}

static void rb7_fx_box(uint8_t id, int32_t x, int32_t y, uint8_t selected)
{
    static const char *const name[6] = {"DRIVE","COMP","FILTER","DLY FB","DLY MIX","DLY TIME"};
    char b[12];
    uint16_t v = refm_felucca_fx_value(id);
    int32_t shown = id == 5u ? (int32_t)(v / 44u) : (int32_t)rb5_pct(v);
    fmt_int(b, shown);
    rb7_pad(x, y, 74, 49, name[id], b, selected ? RB5_AMBER : RB5_PANEL2, selected);
}

'''
    text = replace_once(text, "static void draw_refm_beta(void)\n", helpers + "static void draw_refm_beta(void)\n", "parity helpers")

    home = r'''if (page == 0u) {
        for (i = 0u; i < 4u; ++i) {
            uint16_t lv = refm_felucca_mix_value((uint8_t)i, 0u);
            char v[12];
            fmt_int(v, (int32_t)rb5_pct(lv));
            if (force || i == machine || i == rb5.machine)
                rb7_pad(4 + (int32_t)(i & 1u) * 118, 62 + (int32_t)(i >> 1) * 55,
                        114, 49, RB5_MACHINE[i], v, i == machine ? RB5_AMBER : RB5_PANEL2, i == machine);
        }
        if (force || pattern != rb5.pattern) {
            b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;
            rb5_textbox(4, 177, 55, 40, "PATTERN", b, RB5_RED);
        }
        if (force || play_step != rb5.play_step) rb5_numbox(63,177,55,40,"STEP",(int32_t)play_step+1,RB5_RED);
        if (force || master_pct != rb5.master_pct) rb5_numbox(122,177,55,40,"MASTER",master_pct,RB5_GREEN);
        if (force || playing != rb5.playing) rb5_textbox(181,177,55,40,"STATE",playing?"PLAY":"STOP",playing?RB5_GREEN:RB5_RED);
    } else if (page == 1u) {'''
    text = replace_section(text, "if (page == 0u) {", "} else if (page == 1u) {", home, "HOME")

    acid = r'''} else if (page == 1u) {
        if (force || p0 != rb5.p0) rb5_numbox(3,62,56,42,"CUTOFF",(int32_t)rb5_pct(p0),RB5_RED);
        if (force || p1 != rb5.p1) rb5_numbox(62,62,56,42,"RESO",(int32_t)rb5_pct(p1),RB5_RED);
        if (force || p2 != rb5.p2) rb5_numbox(121,62,56,42,"ENV.MOD",(int32_t)rb5_pct(p2),RB5_RED);
        if (force || p3 != rb5.p3) rb5_numbox(180,62,56,42,"DECAY",(int32_t)rb5_pct(p3),RB5_RED);
        if (force || wave != rb5.wave) rb5_textbox(3,109,56,39,"WAVE",wave?"SQUARE":"SAW",RB5_RED);
        if (force) {
            int32_t tune = (int32_t)refm_felucca_ui_value(25u) - 64;
            rb5_numbox(62,109,56,39,"TUNE",tune,RB5_RED);
            rb5_numbox(121,109,56,39,"ACCENT",(int32_t)refm_felucca_ui_value(26u),RB5_AMBER);
            rb5_textbox(180,109,56,39,"EXT/MOD","PANEL",RB5_CYAN);
        }
        for (i = 0u; i < 16u; ++i) {
            uint32_t packed = refm_felucca_pattern_step((uint8_t)i);
            if (force || packed != rb5.step[i] || i == sel_step || i == rb5.selected_step ||
                (playing && (i == play_step || i == rb5.play_step)))
                rb6_acid_strip_cell(i, packed, sel_step, playing ? play_step : 0xffffu);
            rb5.step[i] = packed;
        }
    } else if (page == 2u) {'''
    text = replace_section(text, "} else if (page == 1u) {", "} else if (page == 2u) {", acid, "ACID")

    step = r'''} else if (page == 2u) {
        for (i = 0u; i < 16u; ++i) {
            uint32_t packed = refm_felucca_pattern_step((uint8_t)i);
            if (force || packed != rb5.step[i] || i == sel_step || i == rb5.selected_step ||
                (playing && (i == play_step || i == rb5.play_step)))
                rb6_acid_strip_cell(i, packed, sel_step, playing ? play_step : 0xffffu);
            rb5.step[i] = packed;
        }
        if (force || p0 != rb5.p0) { char nn[5]; rb5_note_name((uint8_t)p0, nn); rb5_textbox(3,62,56,42,"NOTE",nn,RB5_AMBER); }
        if (force || stype != rb5.stype) rb5_textbox(62,62,56,42,"GATE",stype==0u?"REST":stype==1u?"NOTE":"TIE",RB5_RED);
        if (force || accent != rb5.accent) rb5_textbox(121,62,56,42,"ACCENT",accent?"ON":"OFF",accent?RB5_AMBER:RB5_RED);
        if (force || slide != rb5.slide) rb5_textbox(180,62,56,42,"SLIDE",slide?"ON":"OFF",slide?RB5_CYAN:RB5_RED);
        if (force || p3 != rb5.p3) rb5_numbox(3,109,56,40,"PROB",p3,RB5_RED);
        if (force || sel_step != rb5.selected_step) rb5_numbox(62,109,56,40,"STEP",(int32_t)sel_step+1,RB5_RED);
        if (force) {
            rb5_textbox(121,109,56,40,"KEYS","WRITE",RB5_GREEN);
            rb5_textbox(180,109,56,40,"OCT-/+","PITCH",RB5_CYAN);
        }
    } else if (page == 3u) {'''
    text = replace_section(text, "} else if (page == 2u) {", "} else if (page == 3u) {", step, "SEQ EDIT")

    drum = r'''} else if (page == 3u) {
        for (i = 0u; i < 11u; ++i) {
            uint8_t s;
            if (force || i == lane || i == rb5.lane) {
                cv_begin(23, 11, i == lane ? RB5_TEXT : RB5_DARK);
                cv_rect(1,1,21,9,RB5_PANEL2);
                cv_text(3,0,&AF_S,RB5_DRUM[i],i==lane?RB5_AMBER:RB5_TEXT);
                cv_blit(2,55+(int32_t)i*13);
            }
            for (s = 0u; s < 16u; ++s) {
                uint8_t state = refm_felucca_drum_matrix_value((uint8_t)machine, (uint8_t)i, s);
                uint32_t idx = i * 16u + s;
                if (force || state != rb7_drum_cache[idx] || i == lane || i == rb5.lane ||
                    s == sel_step || s == rb5.selected_step || (playing && (s == play_step || s == rb5.play_step)))
                    rb7_drum_cell((uint8_t)i, s, state, (uint8_t)lane, (uint8_t)sel_step, (uint8_t)playing, (uint8_t)play_step);
                rb7_drum_cache[idx] = state;
            }
        }
        if (force || lane != rb5.lane || dstate != rb5.dstate) {
            rb5_textbox(3,202,73,34,"LANE",lane<11u?RB5_DRUM[lane]:"?",RB5_RED);
            rb5_textbox(80,202,73,34,"STATE",dstate==0u?"OFF":dstate==1u?"HIT":"ACC",RB5_AMBER);
            rb5_textbox(157,202,79,34,"KIT",machine==2u?"808":"909",RB5_CYAN);
        }
    } else if (page == 4u) {'''
    text = replace_section(text, "} else if (page == 3u) {", "} else if (page == 4u) {", drum, "DRUM")

    mixer = r'''} else if (page == 4u) {
        if (force) {
            cv_begin(234, 18, RB5_DARK);
            cv_text(63,2,&AF_S,"LVL",RB5_DIM); cv_text(98,2,&AF_S,"PAN",RB5_DIM);
            cv_text(137,2,&AF_S,"DLY",RB5_DIM); cv_text(175,2,&AF_S,"REV",RB5_DIM); cv_text(208,2,&AF_S,"M/S",RB5_DIM);
            cv_blit(3,53);
        }
        for (i = 0u; i < 4u; ++i) {
            uint8_t changed = force || i == machine || i == rb5.machine;
            uint8_t j;
            for (j = 0u; j < 6u; ++j) {
                uint16_t v = refm_felucca_mix_value((uint8_t)i, j);
                if (v != rb7_mix_cache[i][j]) changed = 1u;
                rb7_mix_cache[i][j] = v;
            }
            if (changed) rb7_mix_row((uint8_t)i, (uint8_t)(i == machine));
        }
        if (force || master_pct != rb5.master_pct) rb5_numbox(3,199,73,37,"MASTER",master_pct,RB5_GREEN);
        if (force) {
            rb5_textbox(80,199,73,37,"K1/K2","LVL/PAN",RB5_CYAN);
            rb5_textbox(157,199,79,37,"K3/K4","DLY/REV",RB5_CYAN);
        }
    } else if (page == 5u) {'''
    text = replace_section(text, "} else if (page == 4u) {", "} else if (page == 5u) {", mixer, "MIXER")

    fx = r'''} else if (page == 5u) {
        for (i = 0u; i < 6u; ++i) {
            uint16_t v = refm_felucca_fx_value((uint8_t)i);
            uint8_t selected = (fx_section == 0u && i == 2u) ||
                               (fx_section == 1u && i >= 3u) ||
                               (fx_section == 2u && i == 0u) ||
                               (fx_section == 3u && i == 1u);
            if (force || v != rb7_fx_cache[i] || fx_section != rb5.fx_section)
                rb7_fx_box((uint8_t)i, 4 + (int32_t)(i % 3u) * 78, 62 + (int32_t)(i / 3u) * 56, selected);
            rb7_fx_cache[i] = v;
        }
        if (force || fx_section != rb5.fx_section)
            rb5_textbox(4,178,74,40,"GROUP",fx_section==0u?"FILTER":fx_section==1u?"DELAY":fx_section==2u?"DRIVE":"COMP",RB5_AMBER);
        if (force) {
            rb5_textbox(82,178,74,40,"ALGO","SELECT",RB5_CYAN);
            rb5_textbox(160,178,76,40,"MIXER","DLY/REV",RB5_GREEN);
        }
    } else if (page == 6u) {'''
    text = replace_section(text, "} else if (page == 5u) {", "} else if (page == 6u) {", fx, "FX")

    song = r'''} else if (page == 6u) {
        for (i = 0u; i < 8u; ++i) {
            char p[2]; p[0] = (char)('A' + i); p[1] = 0;
            rb7_pad(4 + (int32_t)(i & 3u) * 59, 64 + (int32_t)(i >> 2) * 48,
                    55, 42, "PATTERN", p, i == pattern ? RB5_AMBER : RB5_PANEL2, i == pattern);
        }
        if (force || song_loop != rb5.song_loop) rb5_textbox(4,166,74,42,"LOOP",song_loop?"ON":"OFF",song_loop?RB5_GREEN:RB5_RED);
        if (force || song_length != rb5.song_length) rb5_numbox(82,166,74,42,"LENGTH",song_length,RB5_RED);
        if (force || playing != rb5.playing) rb5_textbox(160,166,76,42,"STATE",playing?"PLAY":"STOP",playing?RB5_GREEN:RB5_RED);
    } else if (page == 7u) {'''
    text = replace_section(text, "} else if (page == 6u) {", "} else if (page == 7u) {", song, "SONG")

    project = r'''} else if (page == 7u) {
        for (i = 0u; i < 4u; ++i) {
            char s[4]; fmt_int(s, (int32_t)i + 1);
            rb7_pad(4 + (int32_t)i * 59, 65, 55, 52, "SLOT", s, i == slot ? RB5_AMBER : RB5_PANEL2, i == slot);
        }
        if (force) {
            rb5_textbox(4,126,74,42,"OCT-","LOAD",RB5_GREEN);
            rb5_textbox(82,126,74,42,"OCT+","SAVE",RB5_AMBER);
        }
        if (force || project_result != rb5.project_result)
            rb5_textbox(160,126,76,42,"STATUS",project_result==0u?"READY":"DONE",project_result==0u?RB5_TEXT:RB5_GREEN);
        if (force) rb5_textbox(4,177,232,41,"PROJECT","4 LOCAL SLOTS",RB5_CYAN);
    } else {'''
    text = replace_section(text, "} else if (page == 7u) {", "} else {", project, "PROJECT")

    draw.write_text(text)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    patch_bridge(root)
    patch_renderer(root)
    (root / ".refm-rb240-full-parity").write_text(
        "layout=wasm-rack-hierarchy\n"
        "home=four-machines\nacid=filter+wave+tune+accent+16-step\n"
        "seq=16-step+note+gate+accent+slide+probability\n"
        "drum=11x16-full-matrix\nmixer=four-channels-simultaneous\n"
        "fx=six-master-controls\nsong=patterns-a-h\nproject=four-slots\n"
        "factory-demo=136bpm-csharp-minor-original-rave-set\n"
    )
    print("RB-240 full physical/WASM hierarchy parity applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
