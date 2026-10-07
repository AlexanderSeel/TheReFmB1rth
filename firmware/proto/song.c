// SPDX-License-Identifier: GPL-3.0-only
#include "song.h"
#include <string.h>

void song_init(song_t *song) {
    memset(song, 0, sizeof(*song));
    song->loop = 1u;
}

int song_append(song_t *song, uint8_t pattern, uint8_t repeats) {
    if (song->length >= SONG_MAX_SLOTS || repeats == 0u) return 0;
    song->slot[song->length].pattern = pattern;
    song->slot[song->length].repeats = repeats;
    song->length++;
    return 1;
}

void song_reset(song_t *song) {
    song->position = 0u;
    song->repeat_index = 0u;
}

song_event_t song_advance(song_t *song) {
    song_event_t e;
    memset(&e, 0, sizeof(e));
    if (song->length == 0u || song->position >= song->length) return e;

    const song_slot_t *slot = &song->slot[song->position];
    e.valid = 1u;
    e.pattern = slot->pattern;
    e.slot = song->position;
    e.changed = song->repeat_index == 0u;

    song->repeat_index++;
    if (song->repeat_index >= slot->repeats) {
        song->repeat_index = 0u;
        song->position++;
        if (song->position >= song->length) {
            if (song->loop) {
                song->position = 0u;
                e.wrapped = 1u;
            }
        }
    }
    return e;
}
