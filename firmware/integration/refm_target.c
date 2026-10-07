// SPDX-License-Identifier: GPL-3.0-only
#include "refm_target.h"
#include "../proto/groovebox_midi.h"
#include <string.h>

static uint8_t channel_data_len(uint8_t status) {
    switch (status & 0xF0u) {
        case 0xC0u:
        case 0xD0u:
            return 1u;
        case 0x80u:
        case 0x90u:
        case 0xA0u:
        case 0xB0u:
        case 0xE0u:
            return 2u;
        default:
            return 0u;
    }
}

void refm_target_init(refm_target_t *target, uint16_t bpm) {
    memset(target, 0, sizeof(*target));
    groovebox_init(&target->groovebox, bpm);
    groovebox_midi_init(&target->midi);
}

void refm_target_set_external_clock(refm_target_t *target, uint8_t external) {
    groovebox_set_external_clock(&target->groovebox, external ? 1u : 0u);
}

void refm_target_render_q15(refm_target_t *target, int32_t *interleaved_stereo, uint32_t frames) {
    for (uint32_t i = 0; i < frames; ++i) {
        int16_t l = 0, r = 0;
        groovebox_process(&target->groovebox, &l, &r);
        interleaved_stereo[2u * i] = l;
        interleaved_stereo[2u * i + 1u] = r;
    }
}

void refm_target_midi_byte(refm_target_t *target, uint8_t byte) {
    if (byte >= 0xF8u) {
        groovebox_feed_midi_realtime(&target->groovebox, byte);
        return;
    }
    if (byte & 0x80u) {
        if (byte >= 0xF0u) {
            target->midi_status = 0u;
            target->midi_needed = 0u;
            target->midi_have = 0u;
            return;
        }
        target->midi_status = byte;
        target->midi_needed = channel_data_len(byte);
        target->midi_have = 0u;
        return;
    }
    if (!target->midi_status || !target->midi_needed) return;
    target->midi_data[target->midi_have++] = byte & 0x7Fu;
    if (target->midi_have >= target->midi_needed) {
        uint8_t d1 = target->midi_data[0];
        uint8_t d2 = target->midi_needed > 1u ? target->midi_data[1] : 0u;
        groovebox_handle_channel_midi(&target->groovebox, &target->midi, target->midi_status, d1, d2);
        target->midi_have = 0u;
    }
}

void refm_target_panic(refm_target_t *target) {
    acid303_note_off(&target->groovebox.acid[0]);
    acid303_note_off(&target->groovebox.acid[1]);
    for (uint8_t ch = 0u; ch < 16u; ++ch)
        groovebox_handle_channel_midi(&target->groovebox, &target->midi, (uint8_t)(0xB0u | ch), 123u, 0u);
}

int refm_target_save(const refm_target_t *target, uint8_t *out, size_t capacity, size_t *written) {
    return groovebox_save(&target->groovebox, out, capacity, written);
}

int refm_target_load(refm_target_t *target, const uint8_t *blob, size_t blob_len) {
    int rc = groovebox_load(&target->groovebox, blob, blob_len);
    if (rc == PROJECT_OK) groovebox_midi_init(&target->midi);
    return rc;
}
