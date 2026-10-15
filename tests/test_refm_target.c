// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/integration/refm_target.h"

static void test_audio_block(void) {
    refm_target_t t;
    int32_t out[128 * 2];
    refm_target_init(&t, 128u);
    memset(out, 0x55, sizeof(out));
    refm_target_render_q15(&t, out, 128u);
    for (unsigned i = 0; i < 256u; ++i) assert(out[i] >= -32768 && out[i] <= 32767);
}

#ifdef REFM_FELUCCA_PLATFORM
static void test_physical_staged_audio_probe(void) {
    refm_target_t t;
    int32_t out[256 * 2];
    unsigned stage_nonzero[REFM_DIAG_STAGE_COUNT] = {0u,0u,0u,0u,0u};
    int32_t reference_peak = 0;
    refm_target_init(&t, 128u);
    assert(t.diag_frames_remaining == REFM_DIAG_TOTAL_FRAMES);
    assert(t.diag_stage == 0u);

    while (t.diag_frames_remaining) {
        uint8_t stage = t.diag_stage;
        uint32_t before = t.diag_frames_remaining;
        uint32_t n = before > 256u ? 256u : before;
        refm_target_render_q15(&t, out, n);
        for (uint32_t i = 0; i < n; ++i) {
            int32_t l = out[2u * i];
            int32_t r = out[2u * i + 1u];
            int32_t a = l < 0 ? -l : l;
            assert(l >= -32768 && l <= 32767);
            assert(r >= -32768 && r <= 32767);
            if (l || r) stage_nonzero[stage]++;
            if (stage == 0u) {
                assert(l == r);
                if (a > reference_peak) reference_peak = a;
            }
        }
        assert(t.diag_frames_remaining == before - n);
    }

    assert(t.diag_stage == REFM_DIAG_STAGE_COUNT);
    assert(reference_peak >= 7000 && reference_peak <= 9000);
    for (unsigned s = 0; s < REFM_DIAG_STAGE_COUNT; ++s)
        assert(stage_nonzero[s] > 100u);

    memset(out, 0x55, sizeof(out));
    refm_target_render_q15(&t, out, 64u);
    /* Diagnostics reinitialize a fresh stopped groovebox before normal mode. */
    for (unsigned i = 0; i < 128u; ++i) assert(out[i] == 0);
}
#endif

static void test_running_status_and_realtime(void) {
    refm_target_t t;
    refm_target_init(&t, 120u);
    refm_target_midi_byte(&t, 0x90u);
    refm_target_midi_byte(&t, 60u);
    refm_target_midi_byte(&t, 100u);
    assert(t.groovebox.acid[0].gate != 0u);
    refm_target_midi_byte(&t, 0xF8u);
    refm_target_midi_byte(&t, 64u);
    refm_target_midi_byte(&t, 100u);
    assert(t.groovebox.acid[0].gate != 0u);
    refm_target_midi_byte(&t, 0x80u);
    refm_target_midi_byte(&t, 64u);
    refm_target_midi_byte(&t, 0u);
}

static void test_transport_and_project(void) {
    refm_target_t a, b;
    uint8_t blob[PROJECT_STORE_MAX_PAYLOAD];
    size_t written = 0u;
    refm_target_init(&a, 137u);
    refm_target_set_external_clock(&a, 1u);
    a.groovebox.patterns.acid[0][7][0].note = 71u;
    a.groovebox.patterns.acid[0][7][0].flags = SEQ16_GATE;
    refm_target_midi_byte(&a, 0xFAu);
    assert(a.groovebox.transport.playing != 0u);
    for (unsigned i = 0; i < 6u; ++i) refm_target_midi_byte(&a, 0xF8u);
    assert(a.groovebox.step != 0u);
    assert(refm_target_save(&a, blob, sizeof(blob), &written) == PROJECT_OK);
    assert(written > 2000u);
    refm_target_init(&b, 90u);
    assert(refm_target_load(&b, blob, written) == PROJECT_OK);
    assert(b.groovebox.transport.bpm == a.groovebox.transport.bpm);
    assert(b.groovebox.patterns.acid[0][7][0].note == 71u);
}

int main(void) {
    test_audio_block();
#ifdef REFM_FELUCCA_PLATFORM
    test_physical_staged_audio_probe();
#endif
    test_running_status_and_realtime();
    test_transport_and_project();
    puts("target ABI tests: ok");
    return 0;
}
