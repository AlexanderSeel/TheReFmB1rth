// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_ACID303_H
#define REFM_ACID303_H

#include <stdint.h>

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    int32_t lp1;
    int32_t lp2;
    int32_t env;
    int32_t amp;
    int32_t slide_target_inc;
    uint16_t cutoff;
    uint16_t resonance;
    uint16_t env_mod;
    uint16_t decay;
    uint16_t accent;
    uint16_t drive;
    uint8_t square;
    uint8_t gate;
    uint8_t accented;
    uint8_t sliding;
} acid303_t;

void acid303_init(acid303_t *s);
void acid303_set_note(acid303_t *s, uint8_t midi_note, uint8_t accent, uint8_t slide);
void acid303_note_off(acid303_t *s);
int32_t acid303_process(acid303_t *s);
uint32_t acid303_note_to_phase_inc(uint8_t midi_note);

#endif
