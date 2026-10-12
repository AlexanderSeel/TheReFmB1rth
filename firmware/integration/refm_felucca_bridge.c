// SPDX-License-Identifier: GPL-3.0-only
#include "refm_target.h"
#include "../proto/ui_graph_model.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Narrow ABI used by the Felucca integration overlay. This unit is built
 * separately from Felucca's monolithic felucca.c to avoid internal type-name
 * collisions while retaining a small, auditable integration surface.
 *
 * The FM-1 has only 96 KiB of general .data/.bss RAM. Felucca reserves a
 * separate .pool region specifically for large zero-initialised DSP buffers.
 * Keep the full ReFm runtime and project I/O workspace there on hardware so
 * future UI and transport work does not consume the last bytes of general RAM.
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

static refm_target_t g_refm REFM_POOL;
static uint8_t g_refm_ready;
static uint8_t g_external_clock_seen;
static uint8_t g_panel_playing;

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

static void ensure_ready(void) {
    if (!g_refm_ready) {
        refm_target_init(&g_refm, 128u);
        g_refm_ready = 1u;
    }
}

void refm_felucca_init(uint16_t bpm) {
    refm_target_init(&g_refm, bpm);
    g_refm_ready = 1u;
    g_external_clock_seen = 0u;
    g_panel_playing = 0u;
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

/* Physical FM-1 keyboard -> ReFm engine. Felucca still owns scanning/debounce;
   the overlay calls this only for changed note keys. Track 0/1 are ACID A/B.
   Drum keyboard mapping will be added with the dedicated drum page instead of
   guessing a chromatic-to-GM mapping here. */
void refm_felucca_key_note(uint8_t track, uint8_t note, uint8_t on) {
    uint8_t status;
    if (track >= 2u || note > 127u) return;
    ensure_ready();
    status = (uint8_t)((on ? 0x90u : 0x80u) | track);
    refm_target_midi_byte(&g_refm, status);
    refm_target_midi_byte(&g_refm, note);
    refm_target_midi_byte(&g_refm, on ? 108u : 0u);
}

/* Mirror the physical PLAY state into the ReFm internal transport. This is
   deliberately edge-triggered so the panel can continue to use Felucca's
   proven debounce/UI code while ReFm owns the audible sequencer clock. */
void refm_felucca_panel_transport(uint8_t playing) {
    ensure_ready();
    playing = playing ? 1u : 0u;
    if (playing == g_panel_playing) return;
    g_panel_playing = playing;
    refm_target_set_external_clock(&g_refm, 0u);
    g_external_clock_seen = 0u;
    refm_target_midi_byte(&g_refm, playing ? 0xFAu : 0xFCu);
}

void refm_felucca_use_internal_clock(uint16_t bpm) {
    ensure_ready();
    g_refm.groovebox.transport.bpm = bpm;
    refm_target_set_external_clock(&g_refm, 0u);
    g_external_clock_seen = 0u;
}

void refm_felucca_panic(void) {
    ensure_ready();
    refm_target_panic(&g_refm);
}

/* Called by the pinned Felucca UI overlay after a physical encoder edit.
   Only the stock-like ACID four-knob page is accepted now. The old ADSR/LFO
   hooks remain in the ABI for project compatibility but are intentionally not
   exposed by the panel overlay. */
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
    ui_graph_curve_t g;
    uint32_t i;
    if (track >= 2u || !out || count < UI_GRAPH_POINTS) return 0;
    ensure_ready();
    a = &g_refm.groovebox.acid[track];
    if (kind == REFM_UI_FILTER) {
        ui_graph_filter(&g, (uint8_t)((uint32_t)a->cutoff * 127u / 32767u),
                        (uint8_t)((uint32_t)a->resonance * 127u / 32767u),
                        (uint8_t)((uint32_t)a->env_mod * 127u / 32767u));
    } else {
        return 0;
    }
    for (i = 0; i < UI_GRAPH_POINTS; ++i) out[i] = g.y[i];
    return UI_GRAPH_POINTS;
}

int refm_felucca_save(uint8_t *out, size_t capacity, size_t *written) {
    ensure_ready();
    return refm_target_save(&g_refm, out, capacity, written);
}

int refm_felucca_load(const uint8_t *blob, size_t blob_len) {
    ensure_ready();
    return refm_target_load(&g_refm, blob, blob_len);
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
    return refm_target_load(&g_refm, g_project_io, (size_t)n);
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
