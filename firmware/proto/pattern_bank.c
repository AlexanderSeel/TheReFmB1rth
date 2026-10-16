// SPDX-License-Identifier: GPL-3.0-only
#include "pattern_bank.h"
#include <string.h>

#define HIT(v) ((uint16_t)(1u << (v)))

static void preset_acid(pattern_bank_t *b, uint8_t pat, uint8_t track,
                        const uint8_t notes[SEQ16_STEPS],
                        const uint8_t flags[SEQ16_STEPS])
{
    unsigned s;
    for (s = 0u; s < SEQ16_STEPS; ++s) {
        seq16_step_t *st = &b->acid[track][pat][s];
        st->note = notes[s] ? notes[s] : 37u;
        st->flags = flags[s];
        st->probability = 100u;
        st->micro = 0;
    }
}

static void preset_drums(pattern_bank_t *b, uint8_t pat,
                         const uint16_t d808[SEQ16_STEPS],
                         const uint16_t d909[SEQ16_STEPS])
{
    unsigned s;
    for (s = 0u; s < SEQ16_STEPS; ++s) {
        b->drum_hits[0][pat][s] = d808[s];
        b->drum_hits[1][pat][s] = d909[s];
        b->drum_accents[0][pat][s] = (s == 0u || s == 8u) ? d808[s] : 0u;
        b->drum_accents[1][pat][s] = (s == 0u || s == 4u || s == 8u || s == 12u) ? d909[s] : 0u;
    }
}

/*
 * Factory live set at 136 BPM. Pattern A / ACID A uses the 16-step note,
 * octave and hold sequence supplied by the hardware tester for the intended
 * Confusion performance setup. The remaining A-H material stays available as
 * performance variations around it.
 *
 * DOWN is represented both by the effective lower MIDI note and SEQ16_DOWN so
 * the physical/virtual editors can show the octave switch explicitly. TIE
 * carries Note Held without setting the visible SLIDE flag.
 */
static void pattern_bank_factory_live_set(pattern_bank_t *b)
{
    static const uint8_t a_notes[8][16] = {
        /* A: G/G↓ -> A#/A#↓ user-provided 16-step performance pattern. */
        {43,43,31,43,46,34,34,46,34,46,46,46,46,46,46,46},
        {37,37,44,37,40,37,47,44,37,49,44,40,37,47,44,35},
        {37,44,37,49,40,44,37,47,37,44,40,49,35,40,44,47},
        {37,0,44,47,37,40,44,0,49,47,44,40,37,35,40,44},
        {37,0,0,0,44,0,0,0,40,0,0,0,47,0,44,0},
        {37,0,44,0,40,0,47,0,37,44,40,47,35,40,44,49},
        {37,44,47,49,37,40,44,47,35,40,44,49,37,47,44,40},
        {37,0,44,0,40,0,37,0,35,0,40,0,37,0,0,0}
    };
    static const uint8_t a_flags[8][16] = {
        /* 01 ON, 02 HOLD, 03 HOLD+DOWN, 04 ON, 05 REST,
           06 ON+DOWN, 07 ON+DOWN, 08 REST, 09 ON+DOWN, 10 REST,
           11 ON, 12 HOLD, 13 ON, 14 ON, 15 REST, 16 ON. */
        {SEQ16_GATE,SEQ16_TIE,(uint8_t)(SEQ16_TIE|SEQ16_DOWN),SEQ16_GATE,0,
         (uint8_t)(SEQ16_GATE|SEQ16_DOWN),(uint8_t)(SEQ16_GATE|SEQ16_DOWN),0,
         (uint8_t)(SEQ16_GATE|SEQ16_DOWN),0,SEQ16_GATE,SEQ16_TIE,
         SEQ16_GATE,SEQ16_GATE,0,SEQ16_GATE},
        {3,5,1,1,1,1,5,1,3,1,1,1,5,1,1,1},
        {3,1,5,1,1,5,1,1,3,1,1,5,1,1,1,1},
        {3,0,1,5,1,1,1,0,3,5,1,1,1,1,5,1},
        {3,0,0,0,1,0,0,0,3,0,0,0,1,0,1,0},
        {3,0,1,0,1,0,5,0,3,1,1,5,1,1,1,1},
        {3,1,5,1,3,1,1,5,1,1,3,1,1,5,1,1},
        {3,0,1,0,1,0,1,0,3,0,1,0,1,0,0,0}
    };
    static const uint8_t b_notes[8][16] = {
        {49,0,0,0,56,0,0,0,52,0,0,0,59,0,0,0},
        {49,0,56,0,52,0,59,0,49,0,56,0,52,0,59,0},
        {49,56,0,56,52,59,0,59,49,56,0,56,52,59,0,49},
        {49,0,56,59,52,0,59,0,49,56,52,0,59,0,56,49},
        {49,0,0,0,0,0,0,0,52,0,0,0,0,0,0,0},
        {49,0,56,0,52,0,59,0,49,56,52,59,49,56,59,49},
        {49,56,59,49,52,59,56,49,49,56,59,49,52,59,56,49},
        {49,0,56,0,52,0,49,0,47,0,52,0,49,0,0,0}
    };
    static const uint8_t b_flags[8][16] = {
        {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0},
        {1,5,0,1,1,5,0,1,1,5,0,1,1,5,0,1},
        {1,0,1,5,1,0,1,0,1,5,1,0,1,0,1,5},
        {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
        {1,0,1,0,1,0,1,0,1,5,1,1,1,5,1,1},
        {1,5,1,1,1,5,1,1,1,5,1,1,1,5,1,1},
        {1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0}
    };
    static const uint16_t d808[8][16] = {
        {HIT(0),0,HIT(4),0,0,0,HIT(4),0,HIT(0),0,HIT(4),0,0,0,HIT(4),0},
        {HIT(0),0,HIT(4),0,HIT(1),0,HIT(4),0,HIT(0),0,HIT(4),0,HIT(1),0,HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5)},
        {HIT(0),0,HIT(4),HIT(0),HIT(1),HIT(4),HIT(0)|HIT(4),0,HIT(0),0,HIT(4),HIT(0),HIT(1),HIT(4),HIT(0)|HIT(5),HIT(4)},
        {HIT(0),0,0,0,HIT(1),0,0,0,HIT(0),0,0,0,HIT(1),0,HIT(3),0},
        {HIT(0),0,HIT(4),0,HIT(1),0,HIT(4),HIT(5),HIT(0),0,HIT(4),HIT(0),HIT(1),HIT(4),HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(5)},
        {HIT(0),0,HIT(4),0,HIT(1),0,HIT(4),0,HIT(0),0,HIT(4),0,HIT(1),0,0,0}
    };
    static const uint16_t d909[8][16] = {
        {HIT(0),0,0,0,HIT(0),0,0,HIT(5),HIT(0),0,0,0,HIT(0),0,HIT(4),HIT(5)},
        {HIT(0),0,HIT(4),0,HIT(0)|HIT(1),0,HIT(4),HIT(5),HIT(0),0,HIT(4),0,HIT(0)|HIT(1),0,HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(0)|HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(0)|HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5)},
        {HIT(0),0,HIT(4),HIT(0),HIT(0)|HIT(1),0,HIT(4),HIT(5),HIT(0),HIT(4),HIT(4),0,HIT(0)|HIT(1),0,HIT(4),HIT(10)},
        {HIT(0),0,0,0,HIT(1),0,0,HIT(5),HIT(0),0,0,0,HIT(1)|HIT(9),0,HIT(10),0},
        {HIT(0),0,HIT(4),0,HIT(0)|HIT(1),0,HIT(4),HIT(5),HIT(0),0,HIT(4),HIT(0),HIT(0)|HIT(1),0,HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(0)|HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(4),HIT(0)|HIT(1)|HIT(4),HIT(4),HIT(10),HIT(5)},
        {HIT(0),0,HIT(4),0,HIT(0)|HIT(1),0,HIT(4),0,HIT(0),0,HIT(4),0,HIT(1),0,0,HIT(10)}
    };
    unsigned p;

    for (p = 0u; p < PATTERN_BANK_COUNT; ++p) {
        preset_acid(b, (uint8_t)p, 0u, a_notes[p], a_flags[p]);
        preset_acid(b, (uint8_t)p, 1u, b_notes[p], b_flags[p]);
        preset_drums(b, (uint8_t)p, d808[p], d909[p]);
    }

    b->drum_909_flams[3][15] = HIT(10);
    b->drum_909_flams[5][15] = HIT(5);
    b->drum_909_flams[6][7] = HIT(5);
    b->drum_909_flams[6][15] = HIT(5);
}

void pattern_bank_clear(pattern_bank_t *b, uint8_t p)
{
    if (p >= PATTERN_BANK_COUNT) return;
    for (unsigned t = 0; t < PATTERN_ACID_TRACKS; ++t) {
        memset(b->acid[t][p], 0, sizeof(b->acid[t][p]));
        for (unsigned s = 0; s < SEQ16_STEPS; ++s)
            b->acid[t][p][s].probability = 100u;
    }
    for (unsigned d = 0; d < PATTERN_DRUM_TRACKS; ++d) {
        memset(b->drum_hits[d][p], 0, sizeof(b->drum_hits[d][p]));
        memset(b->drum_accents[d][p], 0, sizeof(b->drum_accents[d][p]));
    }
    memset(b->drum_909_flams[p], 0, sizeof(b->drum_909_flams[p]));
}

void pattern_bank_init(pattern_bank_t *b)
{
    memset(b, 0, sizeof(*b));
    for (unsigned p = 0; p < PATTERN_BANK_COUNT; ++p)
        pattern_bank_clear(b, (uint8_t)p);
    pattern_bank_factory_live_set(b);
}

void pattern_bank_copy(pattern_bank_t *b, uint8_t dst, uint8_t src)
{
    if (dst >= PATTERN_BANK_COUNT || src >= PATTERN_BANK_COUNT) return;
    for (unsigned t = 0; t < PATTERN_ACID_TRACKS; ++t)
        memcpy(b->acid[t][dst], b->acid[t][src], sizeof(b->acid[t][dst]));
    for (unsigned d = 0; d < PATTERN_DRUM_TRACKS; ++d) {
        memcpy(b->drum_hits[d][dst], b->drum_hits[d][src], sizeof(b->drum_hits[d][dst]));
        memcpy(b->drum_accents[d][dst], b->drum_accents[d][src], sizeof(b->drum_accents[d][dst]));
    }
    memcpy(b->drum_909_flams[dst], b->drum_909_flams[src], sizeof(b->drum_909_flams[dst]));
}
