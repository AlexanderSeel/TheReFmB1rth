// SPDX-License-Identifier: GPL-3.0-only
#include "midi_transport.h"
#include <string.h>

#define MT_SAMPLE_RATE 44100u
#define MT_PPQN 24u
#define MT_PULSES_PER_STEP 6u

void midi_transport_init(midi_transport_t *t, uint16_t bpm) {
    memset(t, 0, sizeof(*t));
    midi_transport_set_bpm(t, bpm);
}

void midi_transport_set_bpm(midi_transport_t *t, uint16_t bpm) {
    if (bpm < 30u) bpm = 30u;
    if (bpm > 300u) bpm = 300u;
    t->bpm = bpm;
}

static uint32_t apply_clock(midi_transport_t *t) {
    uint32_t ev = MT_EVT_CLOCK;
    if (!t->playing) return ev;
    t->pulse_in_step++;
    if (t->pulse_in_step >= MT_PULSES_PER_STEP) {
        t->pulse_in_step = 0u;
        ev |= MT_EVT_STEP;
    }
    return ev;
}

uint32_t midi_transport_feed_realtime(midi_transport_t *t, uint8_t status) {
    switch (status) {
        case 0xF8u: return apply_clock(t);
        case 0xFAu:
            t->playing = 1u;
            t->pulse_in_step = 0u;
            return MT_EVT_START;
        case 0xFBu:
            t->playing = 1u;
            return MT_EVT_CONTINUE;
        case 0xFCu:
            t->playing = 0u;
            return MT_EVT_STOP;
        default:
            return 0u;
    }
}

uint32_t midi_transport_process_samples(midi_transport_t *t, uint32_t frames) {
    const uint64_t threshold = (uint64_t)MT_SAMPLE_RATE * 60u;
    uint64_t total = (uint64_t)t->sample_acc + (uint64_t)frames * t->bpm * MT_PPQN;
    uint32_t clocks = (uint32_t)(total / threshold);
    t->sample_acc = (uint32_t)(total % threshold);
    uint32_t ev = 0u;
    while (clocks-- > 0u) ev |= apply_clock(t);
    return ev;
}
