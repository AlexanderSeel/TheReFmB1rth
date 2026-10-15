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

/*
 * Public control/state surface stays compatible with existing projects/UI.
 * The DSP state at the end is a float/no-libm FM-1 core adapted from the
 * strategy used by charlesvestal/fm1-x0x's GPL-3.0 Open303 port: polyBLEP
 * oscillator, measured env-mod mapping, TeeBee TB_303 ladder coefficients,
 * short control intervals and per-sample interpolation.
 */
typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    uint32_t lfo_phase;
    uint32_t slide_target_inc;

    /* Legacy diagnostic mirrors. They now mirror the float core in Q15-ish
       units so existing extended physical diagnostics remain useful. */
    int32_t lp1;
    int32_t lp2;
    int32_t lp3;
    int32_t lp4;
    int32_t resonance_hp_lp;
    int32_t output_hp_x;
    int32_t output_hp_y;
    int32_t post_hp_x;
    int32_t post_hp_y;
    int32_t gain_smooth1;
    int32_t gain_smooth2;
    int32_t coeff_b0;
    int32_t coeff_k;
    int32_t coeff_g;
    int32_t coeff_b0_step;
    int32_t coeff_k_step;
    int32_t coeff_g_step;

    int32_t env;
    int32_t env_rc;
    int32_t amp;
    int32_t accent_env;
    int32_t accent_sweep;

    uint16_t cutoff;
    uint16_t resonance;
    uint16_t env_mod;
    uint16_t decay;
    uint16_t accent;
    uint16_t drive;
    int8_t tune;

    /* Retained for project compatibility. The stock 303 envelope remains the
       audible default; these fields are still persisted/read back. */
    uint8_t amp_attack;
    uint8_t amp_decay;
    uint8_t amp_sustain;
    uint8_t amp_release;
    uint8_t amp_stage;

    uint8_t lfo_rate;
    uint8_t lfo_amount;
    uint8_t lfo_shape;

    uint8_t square;
    uint8_t gate;
    uint8_t accented;
    uint8_t sliding;
    uint8_t idle;
    uint8_t coeff_countdown;
    uint8_t coeff_valid;

    /* FM-1 x0x/Open303-style core. */
    float f_phase;
    float f_freq;
    float f_target_freq;
    float f_slew;
    float f_env;
    float f_env_rc1;
    float f_env_rc2;
    float f_amp;
    float f_amp_smooth;

    float f_b0;
    float f_k;
    float f_g2;
    float f_b0_target;
    float f_k_target;
    float f_g2_target;
    float f_b0_step;
    float f_k_step;
    float f_g2_step;

    float f_hp1_x;
    float f_hp1_y;
    float f_fb_x;
    float f_fb_y;
    float f_y1;
    float f_y2;
    float f_y3;
    float f_y4;
    float f_post_x;
    float f_post_y;

    float f_decim_x1;
    float f_decim_y1;
    float f_last_cutoff;
    float f_last_reso;
    float f_last_envmod;
    uint8_t f_ctrl_left;
} acid303_t;

void acid303_init(acid303_t *s);
void acid303_set_note(acid303_t *s, uint8_t midi_note, uint8_t accent, uint8_t slide);
void acid303_note_off(acid303_t *s);
int32_t acid303_process(acid303_t *s);
uint32_t acid303_note_to_phase_inc(uint8_t midi_note);
uint32_t acid303_apply_tune(uint32_t phase_inc, int8_t tune);
int16_t acid303_lfo_value(const acid303_t *s);
uint16_t acid303_cutoff_hz(const acid303_t *s);

#endif
