#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Physical FM-1 beta integration discovered and refined on real hardware.

The beta deliberately takes ownership of the normal Felucca page/input loop while
keeping Felucca's proven scanner, LCD primitives, USB/MIDI plumbing and updater.
That prevents the first-test failure mode where Felucca controls manipulated one
engine while TheReFmB1rth audio callback rendered another.
"""
from __future__ import annotations
import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"hardware-beta anchor mismatch in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("felucca", type=Path)
    args = ap.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-overlay").is_file():
        raise SystemExit("apply ReFm overlay before hardware beta fixes")

    # The physical keyboard stays on Felucca's proven key scanner/debounce, but
    # its note edges are routed to the currently selected ReFm ACID machine.
    # Editor pages deliberately suppress NOTE ON while always forwarding NOTE OFF.
    seq = root / "firmware" / "src" / "seq.c"
    replace_once(
        seq,
        "static void keyboard_block(void)\n{\n    uint32_t cur = fm1_in.notes, ch, k;\n",
        "static uint32_t refm_prev_panel_notes;\n"
        "extern void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on);\n"
        "extern int refm_felucca_keys_are_editor(void);\n"
        "static void keyboard_block(void)\n{\n"
        "    uint32_t cur = fm1_in.notes, ch, k;\n"
        "    uint32_t refm_changed = cur ^ refm_prev_panel_notes;\n"
        "    if (refm_changed) {\n"
        "        for (k = 0; k < 27u; ++k) {\n"
        "            uint32_t bit = 1u << k;\n"
        "            if (refm_changed & bit) {\n"
        "                uint32_t note = kb_map(TSEL, k);\n"
        "                uint8_t on = (cur & bit) ? 1u : 0u;\n"
        "                if (note <= 127u && (!on || (song.grid == 0u && !refm_felucca_keys_are_editor())))\n"
        "                    refm_felucca_key_note((uint8_t)song.sel, (uint8_t)note, on);\n"
        "            }\n"
        "        }\n"
        "        refm_prev_panel_notes = cur;\n"
        "    }\n"
    )

    # Stop pretending Felucca ENV/LFO pages are ReFm pages. The beta takes over
    # the panel below; this cleanup keeps the fallback integration semantically safe.
    ui = root / "firmware" / "src" / "ui_input.c"
    replace_once(
        ui,
        "        if (pg->fam == FAM_ENV && pg->graph == GR_ADSR) rk = 1u;\n"
        "        else if (pg->fam == FAM_LFO && pg->graph == GR_LFO) rk = 2u;\n"
        "        else if (pg->fam == FAM_LFO && pg->id[slot] == P_LD_FLT) rk = 3u;\n"
        "        else if (pg->fam == FAM_EDIT && pg->id[slot] >= P_E0 && pg->id[slot] <= P_E3) rk = 4u;\n",
        "        if (pg->fam == FAM_EDIT && pg->id[slot] >= P_E0 && pg->id[slot] <= P_E3) rk = 4u;\n"
    )

    graph = root / "firmware" / "src" / "ui_graph.c"
    replace_once(
        graph,
        "        if (song.sel < 2u && pg->fam == FAM_ENV && pg->graph == GR_ADSR) { graph_refm(1u, c); }\n"
        "        else if (song.sel < 2u && pg->fam == FAM_LFO && pg->graph == GR_LFO) { graph_refm(2u, c); }\n"
        "        else if (song.sel < 2u && pg->fam == FAM_EDIT && pg->id[0] == P_E0) { graph_refm(4u, c); }\n"
        "        else switch (pg->graph) {\n",
        "        if (song.sel < 2u && pg->fam == FAM_EDIT && pg->id[0] == P_E0) { graph_refm(4u, c); }\n"
        "        else switch (pg->graph) {\n"
    )

    # Full beta panel takeover. The printed FM-1 labels are reused as memorable
    # shortcuts, while the LCD always explains the current four-knob mapping.
    input_helper = r'''extern void refm_felucca_ui_action(uint8_t action);
extern void refm_felucca_ui_encoder(uint8_t encoder, int16_t delta);
extern uint16_t refm_felucca_ui_value(uint8_t id);

static int refm_beta_input(uint32_t pressed)
{
    int32_t s;
    uint16_t page = refm_felucca_ui_value(0u);
#define RB_HIT(lbl) ((pressed & (1u << panel.btn[(lbl)])) != 0u)
    if (RB_HIT(B_HOME)) refm_felucca_ui_action(0u);       /* HOME */
    if (RB_HIT(B_ENV))  refm_felucca_ui_action(1u);       /* ACID A */
    if (RB_HIT(B_LFO))  refm_felucca_ui_action(2u);       /* ACID B */
    if (RB_HIT(B_SEQ))  refm_felucca_ui_action(3u);       /* STEP */
    if (RB_HIT(B_ARP))  refm_felucca_ui_action(4u);       /* DRUM / 808-909 */
    if (RB_HIT(B_GLO))  refm_felucca_ui_action(5u);       /* MIXER */
    if (RB_HIT(B_FX))   refm_felucca_ui_action(6u);       /* FX */
    if (RB_HIT(B_SCL))  refm_felucca_ui_action(7u);       /* SONG */
    if (RB_HIT(B_SAVE)) refm_felucca_ui_action(8u);       /* PROJECT */
    if (RB_HIT(B_PLAY)) refm_felucca_ui_action(9u);       /* transport */
    if (RB_HIT(B_EDIT)) refm_felucca_ui_action(10u);      /* context: wave/accent/hit/mute */
    if (RB_HIT(B_REC))  refm_felucca_ui_action(11u);      /* alternate: slide/solo */
    if (RB_HIT(B_OCTDN)) {
        if (page == 7u) refm_felucca_ui_action(12u);      /* project LOAD */
        else refm_felucca_ui_encoder(5u, -1);             /* previous machine */
    }
    if (RB_HIT(B_OCTUP)) {
        if (page == 7u) refm_felucca_ui_action(13u);      /* project SAVE */
        else refm_felucca_ui_encoder(5u, 1);              /* next machine */
    }
#undef RB_HIT

    if ((s = panel_enc(EN_K1)) != 0) refm_felucca_ui_encoder(0u, (int16_t)s);
    if ((s = panel_enc(EN_K2)) != 0) refm_felucca_ui_encoder(1u, (int16_t)s);
    if ((s = panel_enc(EN_K3)) != 0) refm_felucca_ui_encoder(2u, (int16_t)s);
    if ((s = panel_enc(EN_K4)) != 0) refm_felucca_ui_encoder(3u, (int16_t)s);
    if ((s = panel_enc(EN_PRESET)) != 0) refm_felucca_ui_encoder(4u, (int16_t)s);
    if ((s = panel_enc(EN_ALGO)) != 0) refm_felucca_ui_encoder(5u, (int16_t)s);
    if ((s = panel_enc(EN_SELECT)) != 0) refm_felucca_ui_encoder(6u, (int16_t)s);

    /* Felucca's own sequencer/record/page state stays stopped. ReFm owns the
       audible transport and editor, which removes the double-engine mismatch. */
    song.grid = 0u;
    song.playing = 0u;
    song.rec = 0u;
    transport_req = 0u;
    return 1;
}

'''
    replace_once(
        ui,
        "static void ui_input(void)\n{\n    uint32_t pressed = fm1_input_edges(0), notes = fm1_input_note_edges(), now = fm1_ticks(), id, b, k;\n",
        input_helper +
        "static void ui_input(void)\n{\n"
        "    uint32_t pressed = fm1_input_edges(0), notes = fm1_input_note_edges(), now = fm1_ticks(), id, b, k;\n"
        "    if (refm_beta_input(pressed)) { (void)notes; (void)now; (void)id; (void)b; (void)k; return; }\n"
    )

    # A real page renderer replaces the old Felucca cards. It redraws only when
    # the compact ReFm view changes, avoiding the lag from repainting 240x240 at
    # every UI frame.
    draw = root / "firmware" / "src" / "ui_draw.c"
    renderer = r'''extern uint16_t refm_felucca_ui_value(uint8_t id);

static const char *const REFM_PAGE_NAME[9] = {
    "HOME", "ACID", "STEP", "DRUM", "MIXER", "FX", "SONG", "PROJECT", "EXT"
};
static const char *const REFM_MACHINE_NAME[4] = {"ACID A", "ACID B", "808", "909"};
static const char *const REFM_DRUM_NAME[11] = {"BD","SD","CP","RS","CH","OH","LT","MT","HT","CR","RD"};

static uint32_t refm_pct(uint32_t v) { return (v * 100u + 16383u) / 32767u; }

static void refm_card_val(int32_t x, int32_t y, const char *label, int32_t value)
{
    char b[12];
    cv_begin(56, 44, T_SURF);
    cv_text(4, 3, &AF_S, label, T_MID);
    fmt_int(b, value);
    cv_text(4, 21, &AF_M, b, T_THEME);
    cv_blit(x, y);
}

static void refm_card_text(int32_t x, int32_t y, const char *label, const char *value)
{
    cv_begin(56, 44, T_SURF);
    cv_text(4, 3, &AF_S, label, T_MID);
    cv_text(4, 21, &AF_M, value, T_THEME);
    cv_blit(x, y);
}

static void refm_footer(const char *text)
{
    cv_begin(240, 28, T_BG);
    cv_text(5, 7, &AF_S, text, T_MID);
    cv_blit(0, 212);
}

static void refm_steps(uint16_t selected, uint16_t playing)
{
    uint32_t i;
    for (i = 0; i < 16u; ++i) {
        int32_t x = 8 + (int32_t)(i & 7u) * 29;
        int32_t y = 77 + (int32_t)(i >> 3) * 27;
        uint16_t c = i == selected ? T_ACCENT : i == playing ? T_THEME : T_SURF;
        lcd_fill(x, y, 22, 18, c);
    }
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
    uint16_t acc = refm_felucca_ui_value(15u), slide = refm_felucca_ui_value(16u);
    uint16_t dstate = refm_felucca_ui_value(18u), mute = refm_felucca_ui_value(19u), solo = refm_felucca_ui_value(20u);
    uint16_t project_result = refm_felucca_ui_value(21u);
    uint32_t sig = page * 2654435761u ^ machine * 2246822519u ^ pattern * 3266489917u ^
                   playing * 668265263u ^ play_step * 374761393u ^ sel_step * 1274126177u ^
                   lane * 42595009u ^ slot * 334214467u ^ bpm * 97u ^ p0 * 3u ^ p1 * 5u ^ p2 * 7u ^ p3 * 11u ^
                   wave * 13u ^ stype * 17u ^ acc * 19u ^ slide * 23u ^ dstate * 29u ^ mute * 31u ^ solo * 37u ^ project_result * 41u;
    static uint32_t last_sig = 0xffffffffu;
    char b[12];
    const char *pn = page < 9u ? REFM_PAGE_NAME[page] : "REFM";
    const char *mn = machine < 4u ? REFM_MACHINE_NAME[machine] : "?";
    if (sig == last_sig) return;
    last_sig = sig;

    lcd_fill(0, 0, 240, 240, T_BG);
    cv_begin(240, 24, T_BG);
    cv_text(5, HEAD_MY, &AF_M, pn, T_THEME);
    cv_text(76, HEAD_MY, &AF_M, mn, T_TEXT);
    fmt_int(b, bpm);
    cv_text(177, HEAD_MY, &AF_M, b, T_THEME);
    cv_text(213, HEAD_SY, &AF_S, playing ? "PLAY" : "STOP", playing ? T_ACCENT : T_MID);
    cv_blit(0, 0);

    cv_begin(240, 22, T_BG);
    cv_text(5, 5, &AF_S, "PAT", T_MID);
    b[0] = (char)('A' + (pattern & 7u)); b[1] = 0;
    cv_text(34, 5, &AF_S, b, T_THEME);
    cv_text(72, 5, &AF_S, "PRESET=PAT  ALGO=TRACK  SELECT=BPM", T_MID);
    cv_blit(0, 27);

    if (page == 0u) {
        refm_card_text(3, 61, "TRACK", mn);
        b[0] = (char)('A' + (pattern & 7u)); b[1] = 0; refm_card_text(62, 61, "PATTERN", b);
        refm_card_val(121, 61, "STEP", (int32_t)play_step + 1);
        refm_card_text(180, 61, "STATE", playing ? "PLAY" : "STOP");
        cv_begin(240, 42, T_BG); cv_text(8, 9, &AF_M, "Keys play selected ACID track", T_TEXT); cv_blit(0, 119);
        refm_footer("ENV=A  LFO=B  ARP=DRUM  SEQ=STEP");
    } else if (page == 1u) {
        refm_card_val(3, 61, "CUTOFF", (int32_t)refm_pct(p0));
        refm_card_val(62, 61, "RES", (int32_t)refm_pct(p1));
        refm_card_val(121, 61, "ENV", (int32_t)refm_pct(p2));
        refm_card_val(180, 61, "DECAY", (int32_t)refm_pct(p3));
        refm_card_text(3, 112, "WAVE", wave ? "SQUARE" : "SAW");
        refm_card_text(62, 112, "EDIT", "WAVE");
        refm_card_text(121, 112, "SEQ", "STEP");
        refm_card_text(180, 112, "KEYS", "LIVE");
        refm_footer("K1 CUT K2 RES K3 ENV K4 DEC | EDIT WAVE");
    } else if (page == 2u) {
        refm_steps(sel_step, playing ? play_step : 0xffffu);
        refm_card_val(3, 139, "NOTE", p0);
        refm_card_text(62, 139, "TYPE", stype == 0u ? "REST" : stype == 1u ? "NOTE" : "TIE");
        refm_card_text(121, 139, "FLAGS", acc && slide ? "A+S" : acc ? "ACC" : slide ? "SLIDE" : "--");
        refm_card_val(180, 139, "PROB", p3);
        refm_footer("PRESET STEP | K1 NOTE K2 TYPE K3 FLAGS K4 PROB");
    } else if (page == 3u) {
        refm_steps(sel_step, playing ? play_step : 0xffffu);
        refm_card_text(3, 139, "LANE", lane < 11u ? REFM_DRUM_NAME[lane] : "?");
        refm_card_text(62, 139, "STATE", dstate == 0u ? "OFF" : dstate == 1u ? "HIT" : "ACC");
        refm_card_val(121, 139, "STEP", (int32_t)sel_step + 1);
        refm_card_text(180, 139, "KIT", machine == 2u ? "808" : "909");
        refm_footer("K1 LANE K2 OFF/HIT/ACC | PRESET STEP");
    } else if (page == 4u) {
        refm_card_val(3, 61, "LEVEL", (int32_t)refm_pct(p0));
        refm_card_val(62, 61, "PAN", ((int32_t)p1 - 32767) * 100 / 32767);
        refm_card_val(121, 61, "DELAY", (int32_t)p2 * 100 / 127);
        refm_card_val(180, 61, "REVERB", (int32_t)p3 * 100 / 127);
        refm_card_text(3, 112, "MUTE", mute ? "ON" : "OFF");
        refm_card_text(62, 112, "SOLO", solo ? "ON" : "OFF");
        refm_card_text(121, 112, "EDIT", "MUTE");
        refm_card_text(180, 112, "REC", "SOLO");
        refm_footer("ALGO TRACK | K1 LVL K2 PAN K3 DLY K4 REV");
    } else if (page == 5u) {
        refm_card_val(3, 61, "DRIVE", (int32_t)refm_pct(p0));
        refm_card_val(62, 61, "DLY FB", (int32_t)p1 * 100 / 31000);
        refm_card_val(121, 61, "REV MIX", (int32_t)refm_pct(p2));
        refm_card_val(180, 61, "FILTER", (int32_t)refm_pct(p3));
        refm_footer("K1 DRIVE K2 DELAY K3 REVERB K4 FILTER");
    } else if (page == 6u) {
        refm_card_text(3, 61, "MODE", "PATTERN");
        refm_card_text(62, 61, "BANK", "A-H");
        refm_card_text(121, 61, "PLAY", playing ? "RUN" : "STOP");
        refm_card_text(180, 61, "EDIT", "LOOP");
        cv_begin(240, 42, T_BG); cv_text(8, 9, &AF_M, "Beta: pattern playback is primary", T_TEXT); cv_blit(0, 119);
        refm_footer("PRESET PATTERN | PLAY START/STOP | EDIT LOOP");
    } else if (page == 7u) {
        refm_card_val(3, 61, "SLOT", (int32_t)slot + 1);
        refm_card_text(62, 61, "OCT-", "LOAD");
        refm_card_text(121, 61, "OCT+", "SAVE");
        refm_card_text(180, 61, "STATUS", project_result == 0u ? "READY" : "DONE");
        cv_begin(240, 42, T_BG); cv_text(8, 9, &AF_M, "PRESET or K1 selects project slot", T_TEXT); cv_blit(0, 119);
        refm_footer("OCT- LOAD   OCT+ SAVE   HOME EXIT");
    } else {
        cv_begin(240, 48, T_BG); cv_text(8, 10, &AF_M, "Reserved for beta diagnostics", T_TEXT); cv_blit(0, 72);
        refm_footer("HOME returns to performance view");
    }
}

'''
    replace_once(
        draw,
        "static void ui_draw_page(uint32_t counting);\nstatic void ui_draw(void)\n{\n",
        renderer + "static void ui_draw_page(uint32_t counting);\nstatic void ui_draw(void)\n{\n"
    )
    replace_once(
        draw,
        "    style_apply();\n",
        "    style_apply();\n    draw_refm_beta();\n    return;\n"
    )

    # Silent ACID voices were still evaluating the 2x ladder every sample. Once
    # the release is fully idle that work is unnecessary on the FM-1.
    acid = root / "firmware" / "refm" / "proto" / "acid303.c"
    replace_once(
        acid,
        "int32_t acid303_process(acid303_t *s) {\n    slide_step(s);\n",
        "int32_t acid303_process(acid303_t *s) {\n"
        "#ifdef REFM_FELUCCA_PLATFORM\n"
        "    if (s->idle && !s->gate && !s->amp) return 0;\n"
        "#endif\n"
        "    slide_step(s);\n"
    )

    (root / ".refm-hardware-beta").write_text(
        "ui=dedicated-refm-pages\n"
        "panel=refm-owned\n"
        "panel_keyboard=selected-acid\n"
        "legacy_adsr_lfo=disabled\n"
        "acid_idle_fastpath=enabled\n"
    )
    print("FM-1 hardware beta panel/LCD/audio integration applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
