#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Final physical FM-1 correction pass after beta.4 hardware validation.

Beta.4 proved that several software-green assumptions were false on the device:
Felucca's keyboard scanner lived behind mix_block(), which ReFm replaces; the
physical MASTER gain lived in that same bypassed mix path; and the LCD patch
avoided a full clear but still rebuilt nearly every visible region for each
parameter change.

This pass is intentionally target-only. Virtual/WASM keeps its richer controls.
The physical target gets:
- direct fm1_in.notes edge delivery from the live audio ISR;
- exact key-up note ownership so octave/page changes cannot leave notes stuck;
- Felucca MASTER-pot gain restored, plus conservative -6 dB target headroom;
- strict contextual machine/button routing;
- OCT-/OCT+ editor navigation, otherwise real keyboard octave shift;
- a compact 240x240 renderer with per-region/per-cell dirty redraws.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-hardware-rework"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"hardware rework anchor mismatch in {path}: expected 1, got {count}: {old[:120]!r}"
        )
    path.write_text(text.replace(old, new, 1))


def patch_audio(root: Path) -> None:
    audio = root / "firmware" / "src" / "audio.c"
    helper = r'''/* ReFm physical input/output boundary.
 * Felucca normally scans musical keys from events_block() -> keyboard_block(),
 * but the ReFm target replaces mix_block(), so that path is intentionally not
 * executed. Read the already-debounced hardware note bitmap here instead: this
 * function runs once per CTL block in the real audio ISR.
 */
#define REFM_HW_HEADROOM_Q15 16384
static uint32_t refm_hw_prev_notes;
static uint8_t refm_hw_note[27];
extern void refm_felucca_key_event(uint8_t key, uint8_t note, uint8_t on);

static void refm_hw_keyboard_scan(void)
{
    uint32_t cur = fm1_in.notes;
    uint32_t changed = cur ^ refm_hw_prev_notes;
    uint32_t k;
    if (!changed)
        return;
    for (k = 0u; k < 27u; ++k) {
        uint32_t bit = 1u << k;
        if (!(changed & bit))
            continue;
        if (cur & bit) {
            int32_t n = 53 + (int32_t)k + 12 * (int32_t)song.octave;
            if (n < 0) n = 0;
            if (n > 127) n = 127;
            refm_hw_note[k] = (uint8_t)n;
            refm_felucca_key_event((uint8_t)k, refm_hw_note[k], 1u);
        } else {
            refm_felucca_key_event((uint8_t)k, refm_hw_note[k], 0u);
        }
    }
    refm_hw_prev_notes = cur;
}

static void refm_hw_output_gain(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t master = fx_usb_fixed ? 4096 : (int32_t)song.master_q12;
    if (master < 0) master = 0;
    if (master > 4096) master = 4096;
    for (i = 0u; i < n * 2u; ++i) {
        int64_t v = ((int64_t)out[i] * master) >> 12;
        v = (v * REFM_HW_HEADROOM_Q15) >> 15;
        if (v < -32768) v = -32768;
        if (v > 32767) v = 32767;
        out[i] = (int32_t)v;
    }
}

'''
    marker = "static void audio_block(int32_t *out, uint32_t n)"
    text = audio.read_text()
    if marker not in text:
        raise SystemExit("audio_block anchor missing after runtime diagnostics")
    text = text.replace(marker, helper + marker, 1)
    old = "    refm_felucca_audio_block(out, n);\n"
    if text.count(old) != 1:
        raise SystemExit("ReFm render anchor missing/duplicated in audio.c")
    text = text.replace(old, "    refm_hw_keyboard_scan();\n    refm_felucca_audio_block(out, n);\n", 1)
    old = "    refm_felucca_emit_state_readback_if_pending();\n"
    if text.count(old) != 1:
        raise SystemExit("state-readback anchor missing/duplicated in audio.c")
    text = text.replace(
        old,
        "    /* Diagnostics above this point observe raw ReFm engine output. */\n"
        "    refm_hw_output_gain(out, n);\n"
        "    refm_felucca_emit_state_readback_if_pending();\n",
        1,
    )
    audio.write_text(text)


def patch_target_bridge(root: Path) -> None:
    bridge = root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"

    helper = r'''static void cycle_machine_context(int8_t delta)
{
    ui240_machine_t next;
    if (!delta) return;
    if (g_ui.page == UI240_ACID_MAIN || g_ui.page == UI240_ACID_STEP) {
        next = g_ui.machine == UI240_MACHINE_ACID2 ? UI240_MACHINE_ACID1 : UI240_MACHINE_ACID2;
        if (g_ui.machine >= UI240_MACHINE_808) next = UI240_MACHINE_ACID1;
        refm_felucca_panic();
        ui240_set_machine(&g_ui, next);
    } else if (g_ui.page == UI240_DRUM) {
        next = g_ui.machine == UI240_MACHINE_909 ? UI240_MACHINE_808 : UI240_MACHINE_909;
        if (g_ui.machine < UI240_MACHINE_808) next = UI240_MACHINE_808;
        refm_felucca_panic();
        ui240_set_machine(&g_ui, next);
    } else if (g_ui.page == UI240_MIXER) {
        int m = (int)g_ui.machine + (delta < 0 ? -1 : 1);
        if (m < 0) m = 3;
        if (m > 3) m = 0;
        refm_felucca_panic();
        ui240_set_machine(&g_ui, (ui240_machine_t)m);
    }
}

'''
    replace_once(
        bridge,
        "void refm_felucca_ui_encoder(uint8_t encoder, int16_t delta)\n{\n",
        helper + "void refm_felucca_ui_encoder(uint8_t encoder, int16_t delta)\n{\n",
    )
    replace_once(
        bridge,
        "        if (encoder == REFM_ENC_ALGO) {\n"
        "            if (g_ui.page == UI240_FX) ui240_cycle_fx_section(&g_ui, d);\n"
        "            else ui240_cycle_machine(&g_ui, d);\n"
        "            continue;\n"
        "        }\n",
        "        if (encoder == REFM_ENC_ALGO) {\n"
        "            if (g_ui.page == UI240_FX) ui240_cycle_fx_section(&g_ui, d);\n"
        "            else cycle_machine_context(d);\n"
        "            continue;\n"
        "        }\n",
    )
    replace_once(
        bridge,
        "    case REFM_ACT_STEP:\n"
        "        ui240_set_page(&g_ui, g_ui.machine >= UI240_MACHINE_808 ? UI240_DRUM : UI240_ACID_STEP);\n"
        "        break;\n",
        "    case REFM_ACT_STEP:\n"
        "        refm_felucca_panic();\n"
        "        if (g_ui.machine >= UI240_MACHINE_808) ui240_set_machine(&g_ui, UI240_MACHINE_ACID1);\n"
        "        ui240_set_page(&g_ui, UI240_ACID_STEP);\n"
        "        break;\n",
    )
    replace_once(
        bridge,
        "    case REFM_ACT_DRUM:\n"
        "        refm_felucca_panic();\n"
        "        if (g_ui.machine < UI240_MACHINE_808) ui240_set_machine(&g_ui, UI240_MACHINE_808);\n"
        "        else ui240_cycle_machine(&g_ui, 1);\n"
        "        ui240_set_page(&g_ui, UI240_DRUM);\n"
        "        break;\n",
        "    case REFM_ACT_DRUM:\n"
        "        refm_felucca_panic();\n"
        "        if (g_ui.page == UI240_DRUM && g_ui.machine == UI240_MACHINE_808)\n"
        "            ui240_set_machine(&g_ui, UI240_MACHINE_909);\n"
        "        else if (g_ui.page == UI240_DRUM && g_ui.machine == UI240_MACHINE_909)\n"
        "            ui240_set_machine(&g_ui, UI240_MACHINE_808);\n"
        "        else\n"
        "            ui240_set_machine(&g_ui, UI240_MACHINE_808);\n"
        "        ui240_set_page(&g_ui, UI240_DRUM);\n"
        "        break;\n",
    )


def patch_panel_input(root: Path) -> None:
    ui = root / "firmware" / "src" / "ui_input.c"
    text = ui.read_text()
    start_marker = "    if (RB_HIT(B_OCTDN)) {\n"
    end_marker = "#undef RB_HIT\n"
    start = text.find(start_marker)
    end = text.find(end_marker, start)
    if start < 0 or end < 0:
        raise SystemExit("OCT hardware input block not found")
    replacement = r'''    if (RB_HIT(B_OCTDN)) {
        if (page == 2u) refm_felucca_ui_encoder(0u, -1);      /* STEP pitch - */
        else if (page == 3u) refm_felucca_ui_encoder(0u, -1); /* DRUM lane - */
        else if (page == 7u) refm_felucca_ui_action(12u);     /* PROJECT load */
        else if (song.octave > -3) { refm_felucca_panic(); song.octave--; }
    }
    if (RB_HIT(B_OCTUP)) {
        if (page == 2u) refm_felucca_ui_encoder(0u, 1);       /* STEP pitch + */
        else if (page == 3u) refm_felucca_ui_encoder(0u, 1);  /* DRUM lane + */
        else if (page == 7u) refm_felucca_ui_action(13u);     /* PROJECT save */
        else if (song.octave < 3) { refm_felucca_panic(); song.octave++; }
    }
'''
    text = text[:start] + replacement + text[end:]
    ui.write_text(text)


def patch_renderer(root: Path) -> None:
    draw = root / "firmware" / "src" / "ui_draw.c"
    text = draw.read_text()
    start_marker = "extern uint16_t refm_felucca_ui_value(uint8_t id);\n"
    end_marker = "static void ui_draw(void)\n"
    start = text.find(start_marker)
    end = text.find(end_marker, start)
    if start < 0 or end < 0 or end <= start:
        raise SystemExit("dedicated ReFm renderer block not found")

    renderer = r'''extern uint16_t refm_felucca_ui_value(uint8_t id);
extern uint32_t refm_felucca_pattern_step(uint8_t step);

/* Hardware-only RB-240 clean renderer. It deliberately favours legibility and
 * dirty rectangles over desktop-like density. RGB565 colours are self-contained
 * so older skin passes cannot leak obsolete layout assumptions into this pass. */
#define RB5_BG      0x18E3u
#define RB5_PANEL   0x9CF3u
#define RB5_PANEL2  0x632Cu
#define RB5_DARK    0x18E3u
#define RB5_BLACK   0x0841u
#define RB5_LIGHT   0xD69Au
#define RB5_TEXT    0xFFFFu
#define RB5_INK     0x1082u
#define RB5_DIM     0xBDF7u
#define RB5_RED     0xF986u
#define RB5_AMBER   0xFD20u
#define RB5_CYAN    0x2D7Fu
#define RB5_GREEN   0x4FE8u

static const char *const RB5_PAGE[9] = {"HOME","ACID","STEP","DRUM","MIX","FX","SONG","SAVE","EXT"};
static const char *const RB5_MACHINE[4] = {"ACID A","ACID B","808","909"};
static const char *const RB5_DRUM[11] = {"BD","SD","CP","RS","CH","OH","LT","MT","HT","CR","RD"};

struct rb5_cache {
    uint16_t page, machine, pattern, playing, bpm, play_step, selected_step, lane, slot;
    uint16_t p0, p1, p2, p3, wave, stype, accent, slide, dstate, mute, solo;
    uint16_t project_result, fx_section, song_loop, song_length, master_pct;
    uint32_t step[16];
    uint8_t valid;
};
static struct rb5_cache rb5;

static uint32_t rb5_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }

static void rb5_textbox(int32_t x, int32_t y, int32_t w, int32_t h,
                        const char *label, const char *value, uint16_t accent)
{
    cv_begin(w, h, RB5_PANEL);
    cv_rect(0, 0, w, 1, RB5_LIGHT);
    cv_rect(0, h - 1, w, 1, RB5_DARK);
    cv_text(4, 3, &AF_S, label, RB5_INK);
    cv_rect(4, 17, w - 8, h - 21, RB5_BLACK);
    cv_text(7, 19, &AF_S, value, accent);
    cv_blit(x, y);
}

static void rb5_numbox(int32_t x, int32_t y, int32_t w, int32_t h,
                       const char *label, int32_t value, uint16_t accent)
{
    char b[12];
    fmt_int(b, value);
    rb5_textbox(x, y, w, h, label, b, accent);
}

static void rb5_header(uint16_t page, uint16_t machine, uint16_t bpm, uint16_t playing)
{
    char b[12];
    const char *pn = page < 9u ? RB5_PAGE[page] : "REFM";
    const char *mn = machine < 4u ? RB5_MACHINE[machine] : "?";
    if (page == 6u) mn = "PATTERN";
    else if (page == 7u) mn = "PROJECT";
    cv_begin(240, 24, RB5_PANEL);
    cv_rect(0, 0, 240, 1, RB5_LIGHT);
    cv_rect(0, 23, 240, 1, RB5_DARK);
    cv_rect(4, 4, 51, 16, RB5_AMBER);
    cv_text(7, 5, &AF_S, pn, RB5_INK);
    cv_text(61, 5, &AF_S, mn, RB5_INK);
    cv_rect(150, 4, 42, 16, RB5_BLACK);
    fmt_int(b, bpm);
    cv_text(157, 5, &AF_S, b, RB5_RED);
    cv_rect(196, 4, 40, 16, RB5_DARK);
    cv_text(201, 5, &AF_S, playing ? "PLAY" : "STOP", playing ? RB5_GREEN : RB5_TEXT);
    cv_blit(0, 0);
}

static void rb5_pattern_row(uint16_t pattern, uint16_t page, uint16_t selected, uint16_t lane)
{
    char b[12];
    cv_begin(240, 24, RB5_PANEL2);
    cv_rect(0, 0, 240, 1, RB5_LIGHT);
    cv_text(5, 5, &AF_S, "PAT", RB5_INK);
    b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;
    cv_rect(31, 4, 20, 16, RB5_BLACK);
    cv_text(37, 5, &AF_S, b, RB5_RED);
    if (page == 2u || page == 3u) {
        cv_text(63, 5, &AF_S, "STEP", RB5_INK);
        fmt_int(b, (int32_t)selected + 1);
        cv_rect(96, 4, 24, 16, RB5_BLACK);
        cv_text(101, 5, &AF_S, b, RB5_RED);
    }
    if (page == 3u) {
        cv_text(135, 5, &AF_S, "LANE", RB5_INK);
        cv_rect(170, 4, 35, 16, RB5_BLACK);
        cv_text(176, 5, &AF_S, lane < 11u ? RB5_DRUM[lane] : "?", RB5_RED);
    }
    cv_blit(0, 27);
}

static void rb5_note_name(uint8_t note, char out[5])
{
    static const char *const nn[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const char *n = nn[note % 12u];
    int32_t oct = (int32_t)(note / 12u) - 1;
    uint8_t p = 0u;
    out[p++] = n[0];
    if (n[1]) out[p++] = n[1];
    if (oct < 0) out[p++] = '-';
    else out[p++] = (char)('0' + (oct > 9 ? 9 : oct));
    out[p] = 0;
}

static void rb5_acid_cell(uint32_t i, uint32_t packed, uint16_t selected, uint16_t playing)
{
    uint8_t note = (uint8_t)(packed & 0xffu);
    uint8_t flags = (uint8_t)((packed >> 8) & 0xffu);
    uint8_t gate = (uint8_t)((flags & 1u) != 0u);
    uint8_t acc = (uint8_t)((flags & 2u) != 0u);
    uint8_t slide = (uint8_t)((flags & 4u) != 0u);
    uint8_t tie = (uint8_t)((flags & 8u) != 0u && !gate);
    int32_t x = 4 + (int32_t)(i & 7u) * 29;
    int32_t y = 61 + (int32_t)(i >> 3) * 39;
    uint16_t fill = acc && gate ? RB5_RED : gate ? RB5_AMBER : tie ? RB5_CYAN : RB5_PANEL;
    char b[5];
    cv_begin(27, 34, RB5_PANEL2);
    cv_rect(0, 0, 27, 1, i == selected ? RB5_TEXT : RB5_DARK);
    cv_rect(0, 33, 27, 1, i == selected ? RB5_TEXT : RB5_DARK);
    cv_rect(2, 3, 23, 25, fill);
    if (gate) rb5_note_name(note, b);
    else if (tie) { b[0]='T'; b[1]='I'; b[2]='E'; b[3]=0; }
    else { b[0]='-'; b[1]='-'; b[2]=0; }
    cv_text(4, 9, &AF_S, b, (gate || tie) ? RB5_INK : RB5_DIM);
    if (slide) cv_rect(3, 29, 21, 2, RB5_CYAN);
    if (i == playing) cv_rect(24, 3, 2, 27, RB5_GREEN);
    cv_blit(x, y);
}

static void rb5_drum_cell(uint32_t i, uint32_t packed, uint16_t selected, uint16_t playing)
{
    uint8_t state = (uint8_t)(packed & 3u);
    uint8_t flam = (uint8_t)((packed & 4u) != 0u);
    int32_t x = 4 + (int32_t)(i & 7u) * 29;
    int32_t y = 61 + (int32_t)(i >> 3) * 39;
    uint16_t fill = state == 2u ? RB5_RED : state == 1u ? RB5_AMBER : RB5_PANEL;
    char b[4];
    cv_begin(27, 34, RB5_PANEL2);
    cv_rect(0, 0, 27, 1, i == selected ? RB5_TEXT : RB5_DARK);
    cv_rect(0, 33, 27, 1, i == selected ? RB5_TEXT : RB5_DARK);
    cv_rect(2, 3, 23, 25, fill);
    b[0] = state == 2u ? 'A' : state == 1u ? 'X' : '-'; b[1] = 0;
    cv_text(9, 9, &AF_S, b, state ? RB5_INK : RB5_DIM);
    if (flam) cv_rect(4, 29, 19, 2, RB5_CYAN);
    if (i == playing) cv_rect(24, 3, 2, 27, RB5_GREEN);
    cv_blit(x, y);
}

static void draw_refm_beta(void)
{
    uint16_t page = refm_felucca_ui_value(0u), machine = refm_felucca_ui_value(1u);
    uint16_t pattern = refm_felucca_ui_value(2u), playing = refm_felucca_ui_value(3u);
    uint16_t play_step = refm_felucca_ui_value(4u), sel_step = refm_felucca_ui_value(5u);
    uint16_t lane = refm_felucca_ui_value(6u), slot = refm_felucca_ui_value(7u), bpm = refm_felucca_ui_value(8u);
    uint16_t p0 = refm_felucca_ui_value(9u), p1 = refm_felucca_ui_value(10u);
    uint16_t p2 = refm_felucca_ui_value(11u), p3 = refm_felucca_ui_value(12u);
    uint16_t wave = refm_felucca_ui_value(13u), stype = refm_felucca_ui_value(14u);
    uint16_t accent = refm_felucca_ui_value(15u), slide = refm_felucca_ui_value(16u);
    uint16_t dstate = refm_felucca_ui_value(18u), mute = refm_felucca_ui_value(19u), solo = refm_felucca_ui_value(20u);
    uint16_t project_result = refm_felucca_ui_value(21u), fx_section = refm_felucca_ui_value(22u);
    uint16_t song_loop = refm_felucca_ui_value(27u), song_length = refm_felucca_ui_value(28u);
    uint16_t master_pct = (uint16_t)(((song.master_q12 > 4096u ? 4096u : song.master_q12) * 100u) / 4096u);
    uint8_t force = (uint8_t)(!rb5.valid || page != rb5.page || machine != rb5.machine);
    char b[12];
    uint32_t i;

    if (force) lcd_fill(0, 0, 240, 240, RB5_BG);

    if (force || page != rb5.page || machine != rb5.machine || bpm != rb5.bpm || playing != rb5.playing)
        rb5_header(page, machine, bpm, playing);
    if (force || pattern != rb5.pattern || sel_step != rb5.selected_step || lane != rb5.lane)
        rb5_pattern_row(pattern, page, sel_step, lane);

    if (page == 0u) {
        if (force || machine != rb5.machine) rb5_textbox(3, 66, 56, 42, "TRACK", machine < 4u ? RB5_MACHINE[machine] : "?", RB5_RED);
        if (force || pattern != rb5.pattern) { b[0]=(char)('A'+(pattern&7u)); b[1]=0; rb5_textbox(62,66,56,42,"PATTERN",b,RB5_RED); }
        if (force || play_step != rb5.play_step) rb5_numbox(121,66,56,42,"STEP",(int32_t)play_step+1,RB5_RED);
        if (force || playing != rb5.playing) rb5_textbox(180,66,56,42,"STATE",playing?"PLAY":"STOP",playing?RB5_GREEN:RB5_RED);
        if (force) {
            rb5_textbox(3,118,56,42,"ENV","ACID A",RB5_RED);
            rb5_textbox(62,118,56,42,"LFO","ACID B",RB5_RED);
            rb5_textbox(121,118,56,42,"SEQ","STEP",RB5_RED);
            rb5_textbox(180,118,56,42,"ARP","DRUM",RB5_RED);
        }
    } else if (page == 1u) {
        if (force || p0 != rb5.p0) rb5_numbox(3,66,56,42,"CUTOFF",(int32_t)rb5_pct(p0),RB5_RED);
        if (force || p1 != rb5.p1) rb5_numbox(62,66,56,42,"RES",(int32_t)rb5_pct(p1),RB5_RED);
        if (force || p2 != rb5.p2) rb5_numbox(121,66,56,42,"ENV",(int32_t)rb5_pct(p2),RB5_RED);
        if (force || p3 != rb5.p3) rb5_numbox(180,66,56,42,"DECAY",(int32_t)rb5_pct(p3),RB5_RED);
        if (force || wave != rb5.wave) rb5_textbox(3,118,56,42,"WAVE",wave?"SQUARE":"SAW",RB5_RED);
        if (force) {
            rb5_textbox(62,118,56,42,"SEQ","STEP",RB5_RED);
            rb5_textbox(121,118,56,42,"KEYS","LIVE",RB5_GREEN);
            rb5_textbox(180,118,56,42,"OCT","-/+",RB5_RED);
        }
    } else if (page == 2u) {
        for (i = 0u; i < 16u; ++i) {
            uint32_t packed = refm_felucca_pattern_step((uint8_t)i);
            if (force || packed != rb5.step[i] || i == sel_step || i == rb5.selected_step ||
                (playing && (i == play_step || i == rb5.play_step)))
                rb5_acid_cell(i, packed, sel_step, playing ? play_step : 0xffffu);
            rb5.step[i] = packed;
        }
        if (force || p0 != rb5.p0) rb5_numbox(3,145,56,42,"NOTE",p0,RB5_RED);
        if (force || stype != rb5.stype) rb5_textbox(62,145,56,42,"TYPE",stype==0u?"REST":stype==1u?"NOTE":"TIE",RB5_RED);
        if (force || accent != rb5.accent) rb5_textbox(121,145,56,42,"ACC",accent?"ON":"OFF",accent?RB5_AMBER:RB5_RED);
        if (force || slide != rb5.slide) rb5_textbox(180,145,56,42,"SLIDE",slide?"ON":"OFF",slide?RB5_CYAN:RB5_RED);
        if (force || p3 != rb5.p3) rb5_numbox(3,194,56,42,"PROB",p3,RB5_RED);
        if (force || sel_step != rb5.selected_step) rb5_numbox(62,194,56,42,"STEP",(int32_t)sel_step+1,RB5_RED);
        if (force) {
            rb5_textbox(121,194,56,42,"KEY","WRITE",RB5_GREEN);
            rb5_textbox(180,194,56,42,"OCT","PITCH",RB5_RED);
        }
    } else if (page == 3u) {
        for (i = 0u; i < 16u; ++i) {
            uint32_t packed = refm_felucca_pattern_step((uint8_t)i);
            if (force || packed != rb5.step[i] || i == sel_step || i == rb5.selected_step ||
                (playing && (i == play_step || i == rb5.play_step)))
                rb5_drum_cell(i, packed, sel_step, playing ? play_step : 0xffffu);
            rb5.step[i] = packed;
        }
        if (force || lane != rb5.lane) rb5_textbox(3,145,56,42,"LANE",lane<11u?RB5_DRUM[lane]:"?",RB5_RED);
        if (force || dstate != rb5.dstate) rb5_textbox(62,145,56,42,"STATE",dstate==0u?"OFF":dstate==1u?"HIT":"ACC",RB5_RED);
        if (force || sel_step != rb5.selected_step) rb5_numbox(121,145,56,42,"STEP",(int32_t)sel_step+1,RB5_RED);
        if (force || machine != rb5.machine) rb5_textbox(180,145,56,42,"KIT",machine==2u?"808":"909",RB5_RED);
        if (force) {
            rb5_textbox(3,194,56,42,"KEY","HIT",RB5_GREEN);
            rb5_textbox(62,194,56,42,"OCT","LANE",RB5_RED);
            rb5_textbox(121,194,56,42,"K2","LEVEL",RB5_RED);
            rb5_textbox(180,194,56,42,"K3","FLAM",RB5_CYAN);
        }
    } else if (page == 4u) {
        if (force || p0 != rb5.p0) rb5_numbox(3,66,56,42,"LEVEL",(int32_t)rb5_pct(p0),RB5_RED);
        if (force || p1 != rb5.p1) rb5_numbox(62,66,56,42,"PAN",((int32_t)p1-32767)*100/32767,RB5_RED);
        if (force || p2 != rb5.p2) rb5_numbox(121,66,56,42,"DELAY",(int32_t)p2*100/127,RB5_RED);
        if (force || p3 != rb5.p3) rb5_numbox(180,66,56,42,"REVERB",(int32_t)p3*100/127,RB5_RED);
        if (force || mute != rb5.mute) rb5_textbox(3,118,56,42,"MUTE",mute?"ON":"OFF",mute?RB5_AMBER:RB5_RED);
        if (force || solo != rb5.solo) rb5_textbox(62,118,56,42,"SOLO",solo?"ON":"OFF",solo?RB5_GREEN:RB5_RED);
        if (force || master_pct != rb5.master_pct) rb5_numbox(121,118,115,42,"HW MASTER",master_pct,RB5_GREEN);
    } else if (page == 5u) {
        if (force || fx_section != rb5.fx_section) rb5_textbox(3,66,56,42,"SECTION",fx_section==0u?"PCF":fx_section==1u?"DELAY":fx_section==2u?"DIST":"COMP",RB5_RED);
        if (force || p0 != rb5.p0) rb5_numbox(62,66,56,42,"K1",(int32_t)rb5_pct(p0),RB5_RED);
        if (force || p1 != rb5.p1) rb5_numbox(121,66,56,42,"K2",(int32_t)rb5_pct(p1),RB5_RED);
        if (force || p2 != rb5.p2) rb5_numbox(180,66,56,42,"K3",(int32_t)rb5_pct(p2),RB5_RED);
        if (force || p3 != rb5.p3) rb5_numbox(3,118,56,42,"K4",(int32_t)rb5_pct(p3),RB5_RED);
        if (force) rb5_textbox(62,118,174,42,"ALGO","FX SECTION",RB5_CYAN);
    } else if (page == 6u) {
        if (force || pattern != rb5.pattern) { b[0]=(char)('A'+(pattern&7u)); b[1]=0; rb5_textbox(3,66,56,42,"PATTERN",b,RB5_RED); }
        if (force || playing != rb5.playing) rb5_textbox(62,66,56,42,"PLAY",playing?"RUN":"STOP",playing?RB5_GREEN:RB5_RED);
        if (force || song_loop != rb5.song_loop) rb5_textbox(121,66,56,42,"LOOP",song_loop?"ON":"OFF",song_loop?RB5_GREEN:RB5_RED);
        if (force || song_length != rb5.song_length) rb5_numbox(180,66,56,42,"LENGTH",song_length,RB5_RED);
        if (force) rb5_textbox(3,118,233,42,"PRESET","PATTERN A-H",RB5_RED);
    } else if (page == 7u) {
        if (force || slot != rb5.slot) rb5_numbox(3,66,56,42,"SLOT",(int32_t)slot+1,RB5_RED);
        if (force) {
            rb5_textbox(62,66,56,42,"OCT-","LOAD",RB5_GREEN);
            rb5_textbox(121,66,56,42,"OCT+","SAVE",RB5_AMBER);
        }
        if (force || project_result != rb5.project_result) rb5_textbox(180,66,56,42,"STATUS",project_result==0u?"READY":"DONE",RB5_RED);
    } else {
        if (force) rb5_textbox(3,66,233,42,"SYSTEM","DIAGNOSTICS",RB5_RED);
    }

    rb5.page=page; rb5.machine=machine; rb5.pattern=pattern; rb5.playing=playing; rb5.bpm=bpm;
    rb5.play_step=play_step; rb5.selected_step=sel_step; rb5.lane=lane; rb5.slot=slot;
    rb5.p0=p0; rb5.p1=p1; rb5.p2=p2; rb5.p3=p3; rb5.wave=wave; rb5.stype=stype;
    rb5.accent=accent; rb5.slide=slide; rb5.dstate=dstate; rb5.mute=mute; rb5.solo=solo;
    rb5.project_result=project_result; rb5.fx_section=fx_section; rb5.song_loop=song_loop;
    rb5.song_length=song_length; rb5.master_pct=master_pct; rb5.valid=1u;
}

'''
    draw.write_text(text[:start] + renderer + text[end:])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-runtime-diag").is_file():
        raise SystemExit("apply runtime diagnostics before final hardware rework")

    patch_audio(root)
    patch_target_bridge(root)
    patch_panel_input(root)
    patch_renderer(root)

    (root / STAMP).write_text(
        "keyboard=audio-isr-direct-note-edges\n"
        "keyboard-noteoff=owned-note-per-key\n"
        "master=felucca-physical-pot-restored\n"
        "target-headroom=-6db\n"
        "buttons=strict-page-first-contextual\n"
        "lcd=dirty-region-no-page-rebuild-on-knob\n"
        "layout=hardware-240x240-clean-v1\n"
    )
    print("FM-1 physical input/audio/button/LCD rework applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
