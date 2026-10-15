// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_TARGET_H
#define REFM_TARGET_H

#include <stddef.h>
#include <stdint.h>
#include "../proto/groovebox.h"
#include "../proto/midi_router.h"

#define REFM_TARGET_SAMPLE_RATE 44100u
#define REFM_BOOT_PROBE_HZ 1000u
#define REFM_DIAG_STAGE_FRAMES (REFM_TARGET_SAMPLE_RATE * 2u)
#define REFM_DIAG_STAGE_COUNT 5u
#define REFM_DIAG_TOTAL_FRAMES (REFM_DIAG_STAGE_FRAMES * REFM_DIAG_STAGE_COUNT)

typedef struct {
    groovebox_t groovebox;
    midi_router_t midi;
    uint8_t midi_status;
    uint8_t midi_data[2];
    uint8_t midi_needed;
    uint8_t midi_have;
    uint8_t diag_stage;
    uint32_t diag_phase;
    uint32_t diag_stage_frame;
    uint32_t diag_frames_remaining;
    uint16_t diag_bpm;
} refm_target_t;

void refm_target_init(refm_target_t *target, uint16_t bpm);
void refm_target_set_external_clock(refm_target_t *target, uint8_t external);
void refm_target_render_q15(refm_target_t *target, int32_t *interleaved_stereo, uint32_t frames);
void refm_target_midi_byte(refm_target_t *target, uint8_t byte);
void refm_target_panic(refm_target_t *target);
int refm_target_save(const refm_target_t *target, uint8_t *out, size_t capacity, size_t *written);
int refm_target_load(refm_target_t *target, const uint8_t *blob, size_t blob_len);

#endif
