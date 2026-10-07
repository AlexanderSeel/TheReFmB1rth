// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_MIDI_TRANSPORT_H
#define REFM_MIDI_TRANSPORT_H

#include <stdint.h>

#define MT_EVT_CLOCK    (1u << 0)
#define MT_EVT_STEP     (1u << 1)
#define MT_EVT_START    (1u << 2)
#define MT_EVT_CONTINUE (1u << 3)
#define MT_EVT_STOP     (1u << 4)

typedef struct {
    uint16_t bpm;
    uint8_t playing;
    uint8_t pulse_in_step;
    uint32_t sample_acc;
} midi_transport_t;

void midi_transport_init(midi_transport_t *t, uint16_t bpm);
void midi_transport_set_bpm(midi_transport_t *t, uint16_t bpm);
uint32_t midi_transport_feed_realtime(midi_transport_t *t, uint8_t status);
uint32_t midi_transport_process_samples(midi_transport_t *t, uint32_t frames);

#endif
