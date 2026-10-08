// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_SEQ16_H
#define REFM_SEQ16_H
#include <stdint.h>
#define SEQ16_STEPS 16u
enum { SEQ16_GATE=1u<<0, SEQ16_ACCENT=1u<<1, SEQ16_SLIDE=1u<<2, SEQ16_TIE=1u<<3 };
typedef struct { uint8_t note, flags, probability; int8_t micro; } seq16_step_t;
typedef struct { seq16_step_t step[SEQ16_STEPS]; uint8_t current, slide_pending; uint32_t rng; } seq16_t;
typedef struct { uint8_t valid,note_on,note_off,note,accent,slide,tie; } seq16_event_t;
void seq16_init(seq16_t *s, uint32_t seed);
void seq16_reset(seq16_t *s);
seq16_event_t seq16_advance(seq16_t *s);
#endif
