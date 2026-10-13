// SPDX-License-Identifier: GPL-3.0-only
#include "refm_target.h"
#include "../proto/groovebox_pattern.h"
#include "../proto/ui_graph_model.h"
#include "../proto/ui240.h"
#include "../proto/ui240_performance.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Narrow ABI used by the Felucca integration overlay. This unit is built
 * separately from Felucca's monolithic felucca.c to avoid internal type-name
 * collisions while retaining a small, auditable integration surface.
 */
#ifdef REFM_FELUCCA_PLATFORM
#define REFM_POOL __attribute__((section(".pool.refm"), aligned(4)))
#define REFM_PROJECT_IO_CAPACITY 8192u
#else
#define REFM_POOL
#endif

enum {
    REFM_UI_ADSR = 1,
    REFM_UI_LFO = 2,
    REFM_UI_LFO_FILTER = 3,
    REFM_UI_FILTER = 4
};

enum {
    REFM_ACT_HOME = 0,
    REFM_ACT_ACID_A,
    REFM_ACT_ACID_B,
    REFM_ACT_STEP,
    REFM_ACT_DRUM,
    REFM_ACT_MIXER,
    REFM_ACT_FX,
    REFM_ACT_SONG,
    REFM_ACT_PROJECT,
    REFM_ACT_PLAY,
    REFM_ACT_CONTEXT,
    REFM_ACT_ALT,
    REFM_ACT_PROJECT_LOAD,
    REFM_ACT_PROJECT_SAVE,
    REFM_ACT_PANIC
};

enum {
    REFM_ENC_K1 = 0,
    REFM_ENC_K2,
    REFM_ENC_K3,
    REFM_ENC_K4,
    REFM_ENC_PRESET,
    REFM_ENC_ALGO,
    REFM_ENC_SELECT
};

static refm_target_t g_refm REFM_POOL;
static ui240_state_t g_ui;
static uint8_t g_refm_ready;
static uint8_t g_external_clock_seen;
static uint8_t g_panel_playing;
static int8_t g_project_result;
static uint8_t g_modifier;
static uint8_t g_focus_active;
static uint8_t g_focus_region;

#ifdef REFM_FELUCCA_PLATFORM
static uint8_t g_project_io[REFM_PROJECT_IO_CAPACITY] REFM_POOL;
extern int refm_platform_storage_save(uint32_t slot, const void *src, uint32_t len);
extern int refm_platform_storage_load(uint32_t slot, void *dst, uint32_t max);
int refm_felucca_storage_save(uint8_t slot);
int refm_felucca_storage_load(uint8_t slot);
#endif

static uint8_t clamp_u7(int32_t v) {
    if (v < 0) return 0u;
    if (v > 127) return 127u;
    return (uint8_t)v;
}

static int8_t sign_step(int16_t delta) {
    return delta < 0 ? -1 : delta > 0 ? 1 : 0;
}

/* A fresh firmware must make sound when PLAY is pressed. Pattern A is therefore
   a deliberately small factory demonstration, not an implicit test fixture. */
static void seed_beta_pattern(void) {
    groovebox_t *g = &g_refm.groovebox;
    static const uint8_t acid_a[16] = {36,36,43,36, 39,36,46,43, 36,48,43,39, 36,43,46,34};
    static const uint8_t acid_b[16] = {48,0,0,0, 0,0,43,0, 46,0,0,0, 0,0,39,0};
    uint8_t i;
    for (i = 0u; i < 16u; ++i) {
        seq16_step_t *a = &g->acid_seq[0].step[i];
        seq16_step_t *b = &g->acid_seq[1].step[i];
        a->note = acid_a[i]; a->flags = SEQ16_GATE; a->probability = 100u;
        if ((i & 3u) == 0u) a->flags |= SEQ16_ACCENT;
        if (i == 2u || i == 6u || i == 13u) a->flags |= SEQ16_SLIDE;
        b->probability = 100u;
        if (acid_b[i]) { b->note = acid_b[i]; b->flags = (uint8_t)(SEQ16_GATE | ((i == 0u || i == 8u) ? SEQ16_ACCENT : 0u)); }
        else { b->note = 48u; b->flags = 0u; }
        if ((i & 3u) == 0u) g->drum_hits[0][i] |= (uint16_t)(1u << DRUM_BD);
        if (i == 4u || i == 12u) g->drum_hits[0][i] |= (uint16_t)(1u << DRUM_SD);
        if ((i & 1u) == 0u) g->drum_hits[0][i] |= (uint16_t)(1u << DRUM_CH);
        if (i == 0u || i == 8u) g->drum_accents[0][i] |= (uint16_t)(1u << DRUM_BD);
    }
    groovebox_pattern_store(&g->patterns, g, 0u);
}

static void ensure_ready(void) {
    if (!g_refm_ready) {
        refm_target_init(&g_refm, 128u);
        ui240_init(&g_ui);
        seed_beta_pattern();
        g_refm_ready = 1u;
    }
}

void refm_felucca_init(uint16_t bpm) {
    refm_target_init(&g_refm, bpm);
    ui240_init(&g_ui);
    seed_beta_pattern();
    g_refm_ready = 1u;
    g_external_clock_seen = 0u;
    g_panel_playing = 0u;
    g_project_result = 0;
    g_modifier = UI240_MOD_NONE;
    g_focus_active = 0u;
    g_focus_region = UI240_FOCUS_ACID;
}

void refm_felucca_audio_block(int32_t *out, uint32_t frames) { ensure_ready(); refm_target_render_q15(&g_refm, out, frames); }

static uint8_t message_data_len(uint8_t status) {
    uint8_t hi = status & 0xF0u;
    if (hi == 0xC0u || hi == 0xD0u) return 1u;
    if (hi >= 0x80u && hi <= 0xE0u) return 2u;
    return 0u;
}

void refm_felucca_midi_packet(uint32_t packet) {
    uint8_t status = (uint8_t)((packet >> 8) & 0xFFu);
    uint8_t d1 = (uint8_t)((packet >> 16) & 0x7Fu);
    uint8_t d2 = (uint8_t)((packet >> 24) & 0x7Fu);
    uint8_t n;
    ensure_ready();
    if (status >= 0xF8u) {
        if (status == 0xF8u || status == 0xFAu || status == 0xFBu || status == 0xFCu) {
            if (!g_external_clock_seen) { refm_target_set_external_clock(&g_refm, 1u); g_external_clock_seen = 1u; }
            refm_target_midi_byte(&g_refm, status);
        }
        return;
    }
    n = message_data_len(status);
    if (!n) return;
    refm_target_midi_byte(&g_refm, status); refm_target_midi_byte(&g_refm, d1);
    if (n == 2u) refm_target_midi_byte(&g_refm, d2);
}

int refm_felucca_keys_are_editor(void) {
    ensure_ready();
    return g_modifier != UI240_MOD_NONE || g_focus_active || g_ui.page == UI240_ACID_STEP ||
           g_ui.page == UI240_DRUM || g_ui.page == UI240_SONG || g_ui.page == UI240_PROJECT || g_ui.page == UI240_EXT;
}

void refm_felucca_ui_modifier(uint8_t modifier) {
    ensure_ready();
    if (modifier > UI240_MOD_FOCUS) modifier = UI240_MOD_NONE;
    if (modifier == UI240_MOD_FOCUS) {
        if (!g_focus_active) {
            g_focus_active = 1u;
            g_focus_region = UI240_FOCUS_ACID;
            ui240_apply_focus(&g_ui, g_focus_region);
        }
        g_modifier = UI240_MOD_FOCUS;
    } else {
        g_modifier = modifier;
    }
}

void refm_felucca_focus_exit(void) {
    ensure_ready();
    g_focus_active = 0u;
    if (g_modifier == UI240_MOD_FOCUS) g_modifier = UI240_MOD_NONE;
}

void refm_felucca_key_event(uint8_t key, uint8_t note, uint8_t on) {
    ui240_key_result_t r;
    uint8_t status;
    ensure_ready();
    r = ui240_performance_key(&g_ui, &g_refm.groovebox, g_modifier, key, note, on);
    if (r.drum_trigger)
        drum_machine_trigger(&g_refm.groovebox.drum[r.drum_machine], (drum_voice_id_t)r.drum_lane, r.velocity);
    if (r.consumed) return;
    if (g_ui.machine >= UI240_MACHINE_808 || note > 127u) return;
    status = (uint8_t)((on ? 0x90u : 0x80u) | g_ui.machine);
    refm_target_midi_byte(&g_refm, status); refm_target_midi_byte(&g_refm, note); refm_target_midi_byte(&g_refm, on ? 108u : 0u);
}

/* Backward-compatible entry point retained for older overlay tests. */
void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on) { (void)track; refm_felucca_key_event(0xffu, note, on); }

void refm_felucca_panel_transport(uint8_t playing) {
    ensure_ready(); playing = playing ? 1u : 0u;
    if (playing == g_panel_playing && playing == (g_refm.groovebox.transport.playing ? 1u : 0u)) return;
    g_panel_playing = playing; refm_target_set_external_clock(&g_refm, 0u); g_external_clock_seen = 0u;
    refm_target_midi_byte(&g_refm, playing ? 0xFAu : 0xFCu);
}

void refm_felucca_use_internal_clock(uint16_t bpm) { ensure_ready(); midi_transport_set_bpm(&g_refm.groovebox.transport, bpm); refm_target_set_external_clock(&g_refm, 0u); g_external_clock_seen = 0u; }
void refm_felucca_panic(void) { ensure_ready(); refm_target_panic(&g_refm); }

static void select_pattern_delta(int8_t delta) {
    uint8_t next; groovebox_t *g = &g_refm.groovebox;
    if (!delta) return;
    groovebox_pattern_store(&g->patterns, g, g->current_pattern);
    next = (uint8_t)((g->current_pattern + (delta > 0 ? 1u : 7u)) & 7u);
    groovebox_pattern_load(&g->patterns, g, next);
}

static void cycle_step_context(int8_t delta) {
    if (g_ui.machine < UI240_MACHINE_808) ui240_edit_acid_step(&g_ui, &g_refm.groovebox, 1u, delta);
    else ui240_edit_drum(&g_ui, &g_refm.groovebox, 1u, delta);
}

void refm_felucca_ui_encoder(uint8_t encoder, int16_t delta) {
    int8_t d = sign_step(delta); unsigned n;
    ensure_ready(); if (!d) return;
    n = (unsigned)(delta < 0 ? -delta : delta); if (n > 8u) n = 8u;
    while (n--) {
        if (g_focus_active && (encoder == REFM_ENC_PRESET || encoder == REFM_ENC_ALGO)) {
            g_focus_region = ui240_focus_move(&g_ui, g_focus_region, encoder == REFM_ENC_PRESET ? -d : d);
            continue;
        }
        if (encoder == REFM_ENC_PRESET) {
            if (g_ui.page == UI240_ACID_STEP || g_ui.page == UI240_DRUM) ui240_select_step(&g_ui, d);
            else if (g_ui.page == UI240_PROJECT) ui240_select_project(&g_ui, d);
            else select_pattern_delta(d);
            continue;
        }
        if (encoder == REFM_ENC_ALGO) { if (g_ui.page == UI240_FX) ui240_cycle_fx_section(&g_ui, d); else ui240_cycle_machine(&g_ui, d); continue; }
        if (encoder == REFM_ENC_SELECT) {
            int32_t v = (int32_t)g_refm.groovebox.transport.bpm + d;
            if (v < 30) v = 30; if (v > 300) v = 300;
            midi_transport_set_bpm(&g_refm.groovebox.transport, (uint16_t)v); continue;
        }
        if (encoder > REFM_ENC_K4) continue;
        if (g_ui.page == UI240_ACID_MAIN) ui240_edit_acid_main(&g_ui, &g_refm.groovebox, encoder, d);
        else if (g_ui.page == UI240_ACID_STEP) ui240_edit_acid_step(&g_ui, &g_refm.groovebox, encoder, d);
        else if (g_ui.page == UI240_DRUM) ui240_edit_drum(&g_ui, &g_refm.groovebox, encoder, d);
        else if (g_ui.page == UI240_MIXER) ui240_edit_mixer(&g_ui, &g_refm.groovebox, encoder, d);
        else if (g_ui.page == UI240_FX) ui240_edit_fx(&g_ui, &g_refm.groovebox, encoder, d);
        else if (g_ui.page == UI240_PROJECT && encoder == REFM_ENC_K1) ui240_select_project(&g_ui, d);
    }
}

void refm_felucca_ui_action(uint8_t action) {
    groovebox_t *g; ensure_ready(); g = &g_refm.groovebox;
    switch (action) {
    case REFM_ACT_HOME: refm_felucca_panic(); refm_felucca_focus_exit(); ui240_set_page(&g_ui, UI240_HOME); break;
    case REFM_ACT_ACID_A: refm_felucca_panic(); ui240_set_machine(&g_ui, UI240_MACHINE_ACID1); ui240_set_page(&g_ui, UI240_ACID_MAIN); break;
    case REFM_ACT_ACID_B: refm_felucca_panic(); ui240_set_machine(&g_ui, UI240_MACHINE_ACID2); ui240_set_page(&g_ui, UI240_ACID_MAIN); break;
    case REFM_ACT_STEP: ui240_set_page(&g_ui, g_ui.machine >= UI240_MACHINE_808 ? UI240_DRUM : UI240_ACID_STEP); break;
    case REFM_ACT_DRUM: refm_felucca_panic(); if (g_ui.machine < UI240_MACHINE_808) ui240_set_machine(&g_ui, UI240_MACHINE_808); else ui240_cycle_machine(&g_ui, 1); ui240_set_page(&g_ui, UI240_DRUM); break;
    case REFM_ACT_MIXER: ui240_set_page(&g_ui, UI240_MIXER); break;
    case REFM_ACT_FX: if (g_ui.page == UI240_FX) ui240_cycle_fx_section(&g_ui, 1); else ui240_set_page(&g_ui, UI240_FX); break;
    case REFM_ACT_SONG: ui240_set_page(&g_ui, UI240_SONG); break;
    case REFM_ACT_PROJECT: ui240_set_page(&g_ui, UI240_PROJECT); break;
    case REFM_ACT_PLAY: refm_felucca_panel_transport(g->transport.playing ? 0u : 1u); break;
    case REFM_ACT_CONTEXT:
        if (g_ui.page == UI240_ACID_MAIN && g_ui.machine < UI240_MACHINE_808) g->acid[g_ui.machine].square ^= 1u;
        else if (g_ui.page == UI240_ACID_STEP && g_ui.machine < UI240_MACHINE_808) ui240_toggle_acid_flag(&g_ui, g, SEQ16_ACCENT);
        else if (g_ui.page == UI240_DRUM) cycle_step_context(1);
        else if (g_ui.page == UI240_MIXER) g->mixer.track[g_ui.machine & 3u].mute ^= 1u;
        else if (g_ui.page == UI240_FX) ui240_cycle_fx_section(&g_ui, 1);
        else if (g_ui.page == UI240_SONG) g->song.loop ^= 1u;
        break;
    case REFM_ACT_ALT:
        if (g_ui.page == UI240_ACID_STEP && g_ui.machine < UI240_MACHINE_808) ui240_toggle_acid_flag(&g_ui, g, SEQ16_SLIDE);
        else if (g_ui.page == UI240_DRUM && g_ui.machine == UI240_MACHINE_909) ui240_toggle_909_flam(&g_ui, g);
        else if (g_ui.page == UI240_MIXER) g->mixer.track[g_ui.machine & 3u].solo ^= 1u;
        else if (g_ui.page == UI240_FX) ui240_cycle_fx_section(&g_ui, -1);
        break;
    case REFM_ACT_PROJECT_LOAD:
#ifdef REFM_FELUCCA_PLATFORM
        g_project_result = (int8_t)refm_felucca_storage_load(g_ui.project_slot);
#else
        g_project_result = -9;
#endif
        break;
    case REFM_ACT_PROJECT_SAVE:
#ifdef REFM_FELUCCA_PLATFORM
        g_project_result = (int8_t)refm_felucca_storage_save(g_ui.project_slot);
#else
        g_project_result = -9;
#endif
        break;
    case REFM_ACT_PANIC: refm_felucca_panic(); break;
    default: break;
    }
}

uint16_t refm_felucca_ui_value(uint8_t id) {
    ui240_view_t v; ensure_ready(); ui240_snapshot(&g_ui, &g_refm.groovebox, &v);
    switch (id) {
    case 0: return v.page; case 1: return v.machine; case 2: return v.pattern; case 3: return v.playing;
    case 4: return v.playing_step; case 5: return v.selected_step; case 6: return v.selected_lane; case 7: return v.project_slot;
    case 8: return v.bpm; case 9: return v.p0; case 10: return v.p1; case 11: return v.p2; case 12: return v.p3;
    case 13: return v.wave_square; case 14: return v.step_type; case 15: return v.accent; case 16: return v.slide;
    case 17: return v.step_probability; case 18: return v.drum_state; case 19: return v.mute; case 20: return v.solo;
    case 21: return (uint16_t)(uint8_t)g_project_result; case 22: return v.fx_section; case 23: return v.drum_hits; case 24: return v.drum_accents;
    case 25: return (uint16_t)((int16_t)v.acid_tune + 64); case 26: return v.acid_accent; case 27: return v.song_loop; case 28: return v.song_length;
    case 29: return v.acid_cutoff_pct; case 30: return v.acid_resonance_pct; case 31: return v.acid_env_pct; case 32: return v.acid_decay_pct;
    case 33: return g_modifier; case 34: return g_focus_active; case 35: return g_focus_region;
    default: return 0u;
    }
}

void refm_felucca_ui_param(uint8_t track, uint8_t kind, uint8_t slot, int16_t value) {
    acid303_t *a; uint8_t v;
    if (track >= 2u) return; ensure_ready(); a = &g_refm.groovebox.acid[track]; v = clamp_u7(value);
    switch (kind) {
    case REFM_UI_FILTER:
        if (slot == 0u) a->cutoff = (uint16_t)((uint32_t)v * 32767u / 127u);
        else if (slot == 1u) a->resonance = (uint16_t)((uint32_t)v * 32767u / 127u);
        else if (slot == 2u) a->env_mod = (uint16_t)((uint32_t)v * 32767u / 127u);
        else if (slot == 3u) a->decay = (uint16_t)((uint32_t)v * 32767u / 127u);
        break;
    default: break;
    }
}

int refm_felucca_ui_curve(uint8_t track, uint8_t kind, int16_t *out, uint32_t count) {
    acid303_t *a; ui_graph_curve_t graph; uint32_t i;
    if (track >= 2u || !out || count < UI_GRAPH_POINTS) return 0; ensure_ready(); a = &g_refm.groovebox.acid[track];
    if (kind == REFM_UI_FILTER) ui_graph_filter(&graph, (uint8_t)((uint32_t)a->cutoff * 127u / 32767u), (uint8_t)((uint32_t)a->resonance * 127u / 32767u), (uint8_t)((uint32_t)a->env_mod * 127u / 32767u));
    else return 0;
    for (i = 0; i < UI_GRAPH_POINTS; ++i) out[i] = graph.y[i];
    return UI_GRAPH_POINTS;
}

int refm_felucca_save(uint8_t *out, size_t capacity, size_t *written) { ensure_ready(); return refm_target_save(&g_refm, out, capacity, written); }

int refm_felucca_load(const uint8_t *blob, size_t blob_len) {
    int rc; ensure_ready(); rc = refm_target_load(&g_refm, blob, blob_len);
    if (rc == PROJECT_OK) {
        g_panel_playing = g_refm.groovebox.transport.playing ? 1u : 0u;
        if (g_ui.machine >= 4u) g_ui.machine = 0u;
        if (g_ui.fx_section >= UI240_FX_SECTION_COUNT) g_ui.fx_section = UI240_FX_PCF;
        g_modifier = UI240_MOD_NONE; g_focus_active = 0u;
    }
    return rc;
}

#ifdef REFM_FELUCCA_PLATFORM
int refm_felucca_storage_save(uint8_t slot) {
    size_t written = 0u; int rc;
    if (slot >= 4u) return -1; ensure_ready();
    rc = refm_target_save(&g_refm, g_project_io, sizeof(g_project_io), &written); if (rc != PROJECT_OK) return -2;
    return refm_platform_storage_save(slot, g_project_io, (uint32_t)written);
}
int refm_felucca_storage_load(uint8_t slot) {
    int n; if (slot >= 4u) return -1;
    n = refm_platform_storage_load(slot, g_project_io, sizeof(g_project_io)); if (n <= 0) return -2;
    return refm_felucca_load(g_project_io, (size_t)n);
}
#endif

uint16_t refm_felucca_bpm(void) { ensure_ready(); return g_refm.groovebox.transport.bpm; }
uint8_t refm_felucca_pattern(void) { ensure_ready(); return g_refm.groovebox.current_pattern; }
uint8_t refm_felucca_step(void) { ensure_ready(); return g_refm.groovebox.step; }
