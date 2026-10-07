// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_DRUM_MACHINE_H
#define REFM_DRUM_MACHINE_H
#include <stdint.h>
#define DRUM_VOICES 11u
typedef enum { DRUM_BD, DRUM_SD, DRUM_CP, DRUM_RS, DRUM_CH, DRUM_OH, DRUM_LT, DRUM_MT, DRUM_HT, DRUM_CR, DRUM_RD } drum_voice_id_t;
typedef enum { DRUM_MODEL_808, DRUM_MODEL_909 } drum_model_t;
typedef struct { uint32_t phase1, phase2, inc1, inc2; int32_t env, tone; uint8_t active, decay_shift; } drum_voice_state_t;
typedef struct { drum_voice_state_t voice[DRUM_VOICES]; drum_model_t model; uint32_t noise; } drum_machine_t;
void drum_machine_init(drum_machine_t *d, drum_model_t model);
void drum_machine_trigger(drum_machine_t *d, drum_voice_id_t voice, uint8_t velocity);
int16_t drum_machine_process(drum_machine_t *d);
#endif
