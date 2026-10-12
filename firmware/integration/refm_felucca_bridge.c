// SPDX-License-Identifier: GPL-3.0-only
#include "refm_target.h"
#include "../proto/groovebox_pattern.h"
#include "../proto/ui_graph_model.h"
#include "../proto/ui240.h"
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

#ifdef REFM_FELUCCA_PLATFORM
static uint8_t g_project_io[REFM_PROJECT_IO_CAPACITY] REFM_POOL;
extern int refm_platform_storage_save(uint32_t slot, const void *src, uint32_t len);
extern int refm_platform_storage_load(uint32_t slot, void *dst, uint32_t max);
#endif

static uint8_t clamp_u7(int32_t v) {
    if (v < 0) return 0u;
    if (v > 127) return 127u;
    return (uint8_t)v;
}

static int8_t sign_step(int16_t delta) {
    return delta < 0 ? -1 : delta > 0 ? 1 : 0;
}

static void ensure_ready(void) {
    if (!g_refm_ready) {
        refm_target_init(&g_refm, 128u);
        ui240_init(&g_ui);
        g_refm_ready = 1u;
    }
}

void refm_felucca_init(uint16_t bpm) {
    refm_target_init(&g_refm, bpm);
    ui240_init(&g_ui);
    g_refm_ready = 1u;
    g_external_clock_seen = 0u;
    g_panel_playing = 0u;
    g_project_result = 0;
}

void refm_felucca_audio_block(int32_t *out, uint32_t frames) {
    ensure_ready();
    refm_target_render_q15(&g_refm, out, frames);
}

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
            if (!g_external_clock_seen) {
                refm_target_set_external_clock(&g_refm, 1u);
                g_external_clock_seen = 1u;
            }
            refm_target_midi_byte(&g_refm, status);
        }
        return;
    }

    n = message_data_len(status);
    if (!n) return;
    refm_target_midi_byte(&g_refm, status);
    refm_target_midi_byte(&g_refm, d1);
    if (n == 2u) refm_target_midi_byte(&g_refm, d2);
}

int refm_felucca_keys_are_editor(void) {
    ensure_ready();
    return g_ui.page == UI240_ACID_STEP || g_ui.page == UI240_DRUM ||
           g_ui.page == UI240_SONG || g_ui.page == UI240_PROJECT ||
           g_ui.page == UI240_EXT;
}

/* Physical FM-1 keyboard -> selected ReFm ACID engine. The keyboard remains
   playable on HOME/ACID/MIXER/FX and is deliberately disabled on step/project
   editors so editing can never create accidental notes. */
void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on) {
    uint8_t status;
    (void)track;
    ensure_ready();
    if (g_ui.machine >= UI240_MACHINE_808 || note > 127u || refm_felucca_keys_are_editor()) return;
    status = (uint8_t)((on ? 0x90u : 0x80u) | g_ui.machine);
    refm_target_midi_byte(&g_refm, status);
    refm_target_midi_byte(&g_refm, note);
    refm_target_midi_byte(&g_refm, on ? 108u : 0u);
}

void refm_felucca_panel_transport(uint8_t playing) {
    ensure_ready();
    playing = playing ? 1u : 0u;
    if (playing == g_panel_playing && playing == (g_refm.groovebox.transport.playing ? 1u : 0u)) return;
    g_panel_playing = playing;
    refm_target_set_external_clock(&g_refm, 0u);
    g_external_clock_seen = 0u;
    refm_target_midi_byte(&g_refm, playing ? 0xFAu : 0xFCu);
}

void refm_felucca_use_internal_clock(uint16_t bpm) {
    ensure_ready();
    midi_transport_set_bpm(&g_refm.groovebox.transport, bpm);
    refm_target_set_external_clock(&g_refm, 0u);
    g_external_clock_seen = 0u;
}

void refm_felucca_panic(void) {
    ensure_ready();
    refm_target_panic(&g_refm);
}

static void select_pattern_delta(int8_t delta) {
    uint8_t next;
    groovebox_t *g = &g_refm.groovebox;
    if (!delta) return;
    groovebox_pattern_store(&g->patterns, g, g->current_pattern);
    next = (uint8_t)((g->current_pattern + (delta > 0 ? 1u : 7u)) & 7u);
    groovebox_pattern_load(&g->patterns, g, next);
}

static void cycle_step_context(int8_t delta) {
    if (g_ui.machine < UI240_MACHINE_808)
        ui240_edit_acid_step(&g_ui, &g_refm.groovebox, 1u, delta);
    else
        ui240_edit_drum(&g_ui, &g_refm.groovebox, 1u, delta);
}

void refm_felucca_ui_encoder(uint8_t encoder, int16_t delta) {
    int8_t d = sign_step(delta);
    unsigned n;
    ensure_ready();
    if (!d) return;
    n = (unsigned)(delta < 0 ? -delta : delta);
    if (n > 8u) n = 8u;
    while (n--) {
        if (encoder == REFM_ENC_PRESET) {
            if (g_ui.page == UI240_ACID_STEP || g_ui.page == UI240_DRUM) ui240_select_step(&g_ui, d);
            else if (g_ui.page == UI240_PROJECT) ui240_select_project(&g_ui, d);
            else select_pattern_delta(d);
            continue;
        }
        if (encoder == REFM_ENC_ALGO) {
            ui240_cycle_machine(&g_ui, d);
            continue;
        }
        if (encoder == REFM_ENC_SELECT) {
            uint16_t bpm = g_refm.groovebox.transport.bpm;
            int32_t v = (int32_t)bpm + d;
            if (v < 30) v = 30;
            if (v > 300) v = 300;
            midi_transport_set_bpm(&g_refm.groovebox.transport, (uint16_t)v);
            continue;
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
    groovebox_t *g;
    ensure_ready();
    g = &g_refm.groovebox;
    switch (action) {
    case REFM_ACT_HOME:
        ui240_set_page(&g_ui, UI240_HOME);
        break;
    case REFM_ACT_ACID_A:
        ui240_set_machine(&g_ui, UI240_MACHINE_ACID1);
        ui240_set_page(&g_ui, UI240_ACID_MAIN);
        break;
    case REFM_ACT_ACID_B:
        ui240_set_machine(&g_ui, UI240_MACHINE_ACID2);
        ui240_set_page(&g_ui, UI240_ACID_MAIN);
        break;
    case REFM_ACT_STEP:
        if (g_ui.machine >= UI240_MACHINE_808) ui240_set_page(&g_ui, UI240_DRUM);
        else ui240_set_page(&g_ui, UI240_ACID_STEP);
        break;
    case REFM_ACT_DRUM:
        if (g_ui.machine < UI240_MACHINE_808) ui240_set_machine(&g_ui, UI240_MACHINE_808);
        else ui240_cycle_machine(&g_ui, 1);
        ui240_set_page(&g_ui, UI240_DRUM);
        break;
    case REFM_ACT_MIXER:
        ui240_set_page(&g_ui, UI240_MIXER);
        break;
    case REFM_ACT_FX:
        ui240_set_page(&g_ui, UI240_FX);
        break;
    case REFM_ACT_SONG:
        ui240_set_page(&g_ui, UI240_SONG);
        break;
    case REFM_ACT_PROJECT:
        ui240_set_page(&g_ui, UI240_PROJECT);
        break;
    case REFM_ACT_PLAY:
        refm_felucca_panel_transport(g->transport.playing ? 0u : 1u);
        break;
    case REFM_ACT_CONTEXT:
        if (g_ui.page == UI240_ACID_MAIN && g_ui.machine < UI240_MACHINE_808)
            g->acid[g_ui.machine].square ^= 1u;
        else if (g_ui.page == UI240_ACID_STEP && g_ui.machine < UI240_MACHINE_808)
            ui240_toggle_acid_flag(&g_ui, g, SEQ16_ACCENT);
        else if (g_ui.page == UI240_DRUM)
            cycle_step_context(1);
        else if (g_ui.page == UI240_MIXER)
            g->mixer.track[g_ui.machine & 3u].mute ^= 1u;
        else if (g_ui.page == UI240_SONG)
            g->song.loop ^= 1u;
        break;
    case REFM_ACT_ALT:
        if (g_ui.page == UI240_ACID_STEP && g_ui.machine < UI240_MACHINE_808)
            ui240_toggle_acid_flag(&g_ui, g, SEQ16_SLIDE);
        else if (g_ui.page == UI240_MIXER)
            g->mixer.track[g_ui.machine & 3u].solo ^= 1u;
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
    case REFM_ACT_PANIC:
        refm_felucca_panic();
        break;
    default:
        break;
    }
}

/* Stable tiny getter ABI for the monolithic Felucca renderer. Keeping the
   renderer dependent on integers only avoids sharing Felucca-private types. */
uint16_t refm_felucca_ui_value(uint8_t id) {
    ui240_view_t v;
    ensure_ready();
    ui240_snapshot(&g_ui, &g_refm.groovebox, &v);
    switch (id) {
    case 0: return v.page;
    case 1: return v.machine;
    case 2: return v.pattern;
    case 3: return v.playing;
    case 4: return v.playing_step;
    case 5: return v.selected_step;
    case 6: return v.selected_lane;
    case 7: return v.project_slot;
    case 8: return v.bpm;
    case 9: return v.p0;
    case 10: return v.p1;
    case 11: return v.p2;
    case 12: return v.p3;
    case 13: return v.wave_square;
    case 14: return v.step_type;
    case 15: return v.accent;
    case 16: return v.slide;
    case 17: return v.step_probability;
    case 18: return v.drum_state;
    case 19: return v.mute;
    case 20: return v.solo;
    case 21: return (uint16_t)(uint8_t)g_project_result;
    default: return 0u;
    }
}

/* Legacy temporary Felucca parameter hook retained only for compatibility with
   old editor CC paths. Dedicated beta pages use refm_felucca_ui_encoder(). */
void refm_felucca_ui_param(uint8_t track, uint8_t kind, uint8_t slot, int16_t value) {
    acid303_t *a;
    uint8_t v;
    if (track >= 2u) return;
    ensure_ready();
    a = &g_refm.groovebox.acid[track];
    v = clamp_u7(value);
    switch (kind) {
    case REFM_UI_FILTER:
        if (slot == 0u) a->cutoff = (uint16_t)((uint32_t)v * 32767u / 127u);
        else if (slot == 1u) a->resonance = (uint16_t)((uint32_t)v * 32767u / 127u);
        else if (slot == 2u) a->env_mod = (uint16_t)((uint32_t)v * 32767u / 127u);
        else if (slot == 3u) a->decay = (uint16_t)((uint32_t)v * 32767u / 127u);
        break;
    default:
        break;
    }
}

int refm_felucca_ui_curve(uint8_t track, uint8_t kind, int16_t *out, uint32_t count) {
    acid303_t *a;
    ui_graph_curve_t graph;
    uint32_t i;
    if (track >= 2u || !out || count < UI_GRAPH_POINTS) return 0;
    ensure_ready();
    a = &g_refm.groovebox.acid[track];
    if (kind == REFM_UI_FILTER) {
        ui_graph_filter(&graph, (uint8_t)((uint32_t)a->cutoff * 127u / 32767u),
                        (uint8_t)((uint32_t)a->resonance * 127u / 32767u),
                        (uint8_t)((uint32_t)a->env_mod * 127u / 32767u));
    } else {
        return 0;
    }
    for (i = 0; i < UI_GRAPH_POINTS; ++i) out[i] = graph.y[i];
    return UI_GRAPH_POINTS;
}

int refm_felucca_save(uint8_t *out, size_t capacity, size_t *written) {
    ensure_ready();
    return refm_target_save(&g_refm, out, capacity, written);
}

int refm_felucca_load(const uint8_t *blob, size_t blob_len) {
    int rc;
    ensure_ready();
    rc = refm_target_load(&g_refm, blob, blob_len);
    if (rc == PROJECT_OK) {
        g_panel_playing = g_refm.groovebox.transport.playing ? 1u : 0u;
        if (g_ui.machine >= 4u) g_ui.machine = 0u;
    }
    return rc;
}

#ifdef REFM_FELUCCA_PLATFORM
int refm_felucca_storage_save(uint8_t slot) {
    size_t written = 0u;
    int rc;
    if (slot >= 4u) return -1;
    ensure_ready();
    rc = refm_target_save(&g_refm, g_project_io, sizeof(g_project_io), &written);
    if (rc != PROJECT_OK) return -2;
    return refm_platform_storage_save(slot, g_project_io, (uint32_t)written);
}

int refm_felucca_storage_load(uint8_t slot) {
    int n;
    if (slot >= 4u) return -1;
    n = refm_platform_storage_load(slot, g_project_io, sizeof(g_project_io));
    if (n <= 0) return -2;
    return refm_felucca_load(g_project_io, (size_t)n);
}
#endif

uint16_t refm_felucca_bpm(void) {
    ensure_ready();
    return g_refm.groovebox.transport.bpm;
}

uint8_t refm_felucca_pattern(void) {
    ensure_ready();
    return g_refm.groovebox.current_pattern;
}

uint8_t refm_felucca_step(void) {
    ensure_ready();
    return g_refm.groovebox.step;
}
