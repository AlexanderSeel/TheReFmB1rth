// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_PATTERN_BANK_H
#define REFM_PATTERN_BANK_H
#include <stdint.h>
#include "seq16.h"
#define PATTERN_BANK_COUNT 8u
#define PATTERN_ACID_TRACKS 2u
#define PATTERN_DRUM_TRACKS 2u
typedef struct {seq16_step_t acid[PATTERN_ACID_TRACKS][PATTERN_BANK_COUNT][SEQ16_STEPS];uint16_t drum_hits[PATTERN_DRUM_TRACKS][PATTERN_BANK_COUNT][SEQ16_STEPS];uint16_t drum_accents[PATTERN_DRUM_TRACKS][PATTERN_BANK_COUNT][SEQ16_STEPS];} pattern_bank_t;
void pattern_bank_init(pattern_bank_t*b);void pattern_bank_copy(pattern_bank_t*b,uint8_t dst,uint8_t src);void pattern_bank_clear(pattern_bank_t*b,uint8_t pattern);
#endif
