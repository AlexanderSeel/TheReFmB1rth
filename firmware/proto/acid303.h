// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_ACID303_H
#define REFM_ACID303_H

#include <stdint.h>

typedef enum {
    ACID_ENV_OFF = 0,
    ACID_ENV_ATTACK,
    ACID_ENV_DECAY,
    ACID_ENV_SUSTAIN,
    ACID_ENV_RELEASE
} acid_env_stage_t;

typedef enum {
    ACID_LFO_SINE = 0,
    ACID_LFO_TRI,
    ACID_LFO_SAW,
    ACID_LFO_SQUARE
} acid_lfo_shape_t;

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t lfo_phase;
    uint32_t slide_target_inc;

    /* Four coupled stages used by the TB-303/Open303-style diode ladder model. */
    int32_t lp1;
    int32_t lp2;
    int32_t lp3;
    int32_t lp4;
    int32_t resonance_hp_lp;
    int32_t output_hp_x;
    int32_t output_hp_y;

    /* TB-303 style control/envelope state. */
    int32_t env;
    int32_t amp;
    int32_t accent_env;
    int32_t accent_sweep;

    uint16_t cutoff;
    uint16_t resonance;
    uint16_t env_mod;
    uint16_t decay;
    uint16_t accent;
    uint16_t drive;

    /* Retained for project compatibility / optional extended envelope mode.
       Authentic 303 mode uses the fixed VCA contour instead. */
    uint8_t amp_attack;
    uint8_t amp_decay;
    uint8_t amp_sustain;
    uint8_t amp_release;
    uint8_t amp_stage;

    /* Non-stock MOD extension. A stock TB-303 has no LFO. */
    uint8_t lfo_rate;
    uint8_t lfo_amount;
    uint8_t lfo_shape;

    uint8_t square;      /* 0 = saw, 1 = 303-style square */
    uint8_t gate;
    uint8_t accented;
    uint8_t sliding;
} acid303_t;

void acid303_init(acid303_t *s);
void acid303_set_note(acid303_t *s, uint8_t midi_note, uint8_t accent, uint8_t slide);
void acid303_note_off(acid303_t *s);
int32_t acid303_process(acid303_t *s);
uint32_t acid303_note_to_phase_inc(uint8_t midi_note);
int16_t acid303_lfo_value(const acid303_t *s);
uint16_t acid303_cutoff_hz(const acid303_t *s);

#endif
