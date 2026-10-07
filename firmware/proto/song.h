// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_SONG_H
#define REFM_SONG_H

#include <stdint.h>

#define SONG_MAX_SLOTS 64u

typedef struct {
    uint8_t pattern;
    uint8_t repeats;
} song_slot_t;

typedef struct {
    song_slot_t slot[SONG_MAX_SLOTS];
    uint8_t length;
    uint8_t position;
    uint8_t repeat_index;
    uint8_t loop;
} song_t;

typedef struct {
    uint8_t valid;
    uint8_t pattern;
    uint8_t slot;
    uint8_t changed;
    uint8_t wrapped;
} song_event_t;

void song_init(song_t *song);
int song_append(song_t *song, uint8_t pattern, uint8_t repeats);
void song_reset(song_t *song);
song_event_t song_advance(song_t *song);

#endif
