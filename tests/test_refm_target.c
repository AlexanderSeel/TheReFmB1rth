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
static void test_physical_boot_audio_probe(void) {
    refm_target_t t;
    int32_t out[256 * 2];
    int32_t peak = 0;
    unsigned nonzero = 0u;
    refm_target_init(&t, 128u);
    assert(t.diag_frames_remaining == REFM_BOOT_PROBE_FRAMES);

    refm_target_render_q15(&t, out, 256u);
    assert(t.diag_frames_remaining == REFM_BOOT_PROBE_FRAMES - 256u);
    for (unsigned i = 0; i < 256u; ++i) {
        int32_t a = out[2u * i] < 0 ? -out[2u * i] : out[2u * i];
        assert(out[2u * i] == out[2u * i + 1u]);
        if (out[2u * i] != 0) nonzero++;
        if (a > peak) peak = a;
    }
    assert(nonzero > 240u);
    assert(peak >= 7000 && peak <= 9000);

    while (t.diag_frames_remaining) {
        uint32_t n = t.diag_frames_remaining > 256u ? 256u : t.diag_frames_remaining;
        refm_target_render_q15(&t, out, n);
    }
    assert(t.diag_frames_remaining == 0u);

    memset(out, 0x55, sizeof(out));
    refm_target_render_q15(&t, out, 64u);
    /* Fresh stopped groovebox is silent after the boot probe has completed. */
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
    test_physical_boot_audio_probe();
#endif
    test_running_status_and_realtime();
    test_transport_and_project();
    puts("target ABI tests: ok");
    return 0;
}
