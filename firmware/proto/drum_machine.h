// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_DRUM_MACHINE_H
#define REFM_DRUM_MACHINE_H
#include <stdint.h>
#include "sample_voice.h"
#define DRUM_VOICES 11u
typedef enum { DRUM_BD, DRUM_SD, DRUM_CP, DRUM_RS, DRUM_CH, DRUM_OH, DRUM_LT, DRUM_MT, DRUM_HT, DRUM_CR, DRUM_RD } drum_voice_id_t;
typedef enum { DRUM_MODEL_808, DRUM_MODEL_909 } drum_model_t;
typedef struct { uint32_t phase1, phase2, inc1, inc2; int32_t env, tone; uint8_t active, decay_shift; } drum_voice_state_t;
typedef struct {
    drum_voice_state_t voice[DRUM_VOICES];
    sample_voice_t sample_voice[DRUM_VOICES];
    const int16_t *sample_pcm[DRUM_VOICES];
    uint32_t sample_frames[DRUM_VOICES];
    uint32_t sample_rate[DRUM_VOICES];
    uint16_t sample_mask;      /* assets physically available */
    uint16_t sample_use_mask;  /* lanes explicitly routed to samples */
    drum_model_t model;
    uint32_t noise;
    uint8_t samples_enabled;   /* global audition/master switch */
} drum_machine_t;
void drum_machine_init(drum_machine_t *d, drum_model_t model);
void drum_machine_trigger(drum_machine_t *d, drum_voice_id_t voice, uint8_t velocity);
int16_t drum_machine_process(drum_machine_t *d);
void drum_machine_clear_samples(drum_machine_t *d);
void drum_machine_set_sample(drum_machine_t *d, drum_voice_id_t voice, const int16_t *pcm, uint32_t frames, uint32_t sample_rate);
void drum_machine_enable_samples(drum_machine_t *d, uint8_t enabled);
void drum_machine_set_sample_lane(drum_machine_t *d, drum_voice_id_t voice, uint8_t enabled);
void drum_machine_set_sample_use_mask(drum_machine_t *d, uint16_t mask);
uint16_t drum_machine_sample_active_mask(const drum_machine_t *d);
uint8_t drum_machine_sample_available(const drum_machine_t *d, drum_voice_id_t voice);
#endif
