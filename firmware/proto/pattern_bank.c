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
        st->note = notes[s] ? notes[s] : 36u;
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
        b->drum_accents[1][pat][s] = (s == 0u || s == 8u || s == 12u) ? d909[s] : 0u;
    }
}

/*
 * Factory live set: eight original parts designed for dark synth-wave / acid
 * club improvisation. They deliberately evoke the energy of late-80s/90s
 * sequencer music without reproducing any existing song melody.
 *
 * A  intro pulse       E  breakdown
 * B  bass enters       F  rebuild
 * C  driving hats      G  peak
 * D  syncopated push   H  outro / live reset
 */
static void pattern_bank_factory_live_set(pattern_bank_t *b)
{
    static const uint8_t a_notes[8][16] = {
        {36,0,36,0,43,0,39,0,36,0,46,0,43,0,39,0},
        {36,36,43,39,36,46,43,39,36,36,48,46,43,39,34,39},
        {36,43,36,46,39,43,36,48,36,43,39,46,34,39,43,48},
        {36,0,43,46,36,39,43,0,48,46,43,39,36,34,39,43},
        {36,0,0,0,43,0,0,0,39,0,0,0,46,0,43,0},
        {36,0,43,0,39,0,46,0,36,43,39,46,34,39,43,48},
        {36,43,46,48,36,39,43,46,34,39,43,48,36,46,43,39},
        {36,0,43,0,39,0,36,0,34,0,39,0,36,0,0,0}
    };
    static const uint8_t a_flags[8][16] = {
        {3,0,1,0,1,0,1,0,3,0,1,0,1,0,1,0},
        {3,5,1,1,1,1,5,1,3,1,1,1,5,1,1,1},
        {3,1,5,1,1,5,1,1,3,1,1,5,1,1,1,1},
        {3,0,1,5,1,1,1,0,3,5,1,1,1,1,5,1},
        {3,0,0,0,1,0,0,0,3,0,0,0,1,0,1,0},
        {3,0,1,0,1,0,5,0,3,1,1,5,1,1,1,1},
        {3,1,5,1,3,1,1,5,1,1,3,1,1,5,1,1},
        {3,0,1,0,1,0,1,0,3,0,1,0,1,0,0,0}
    };
    static const uint8_t b_notes[8][16] = {
        {48,0,0,0,55,0,0,0,51,0,0,0,58,0,0,0},
        {48,0,55,0,51,0,58,0,48,0,55,0,51,0,58,0},
        {48,55,0,55,51,58,0,58,48,55,0,55,51,58,0,60},
        {48,0,55,58,51,0,58,0,48,55,51,0,58,0,55,60},
        {48,0,0,0,0,0,0,0,51,0,0,0,0,0,0,0},
        {48,0,55,0,51,0,58,0,48,55,51,58,48,55,58,60},
        {48,55,58,60,51,58,55,60,48,55,58,60,51,58,55,60},
        {48,0,55,0,51,0,48,0,46,0,51,0,48,0,0,0}
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
        {HIT(0)|HIT(4),0,HIT(4),0,HIT(1)|HIT(4),0,HIT(4),0,HIT(0)|HIT(4),0,HIT(4),0,HIT(1)|HIT(4),0,HIT(4),0},
        {HIT(0)|HIT(4),0,HIT(4),HIT(0),HIT(1)|HIT(4),0,HIT(4),0,HIT(0)|HIT(4),0,HIT(4),HIT(0),HIT(1)|HIT(4),0,HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5)},
        {HIT(0)|HIT(4),0,HIT(4),HIT(0),HIT(1)|HIT(4),HIT(4),HIT(0)|HIT(4),0,HIT(0)|HIT(4),0,HIT(4),HIT(0),HIT(1)|HIT(4),HIT(4),HIT(0)|HIT(5),HIT(4)},
        {HIT(0),0,0,0,HIT(1),0,0,0,HIT(0),0,0,0,HIT(1),0,HIT(3),0},
        {HIT(0)|HIT(4),0,HIT(4),0,HIT(1)|HIT(4),0,HIT(4),HIT(5),HIT(0)|HIT(4),0,HIT(4),HIT(0),HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(0)|HIT(4),HIT(5)},
        {HIT(0)|HIT(4),0,HIT(4),0,HIT(1)|HIT(4),0,HIT(4),0,HIT(0)|HIT(4),0,HIT(4),0,HIT(1),0,0,0}
    };
    static const uint16_t d909[8][16] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,HIT(4),0,0,0,HIT(4),0,0,0,HIT(4),0,0,0,HIT(4),0},
        {HIT(4),0,HIT(4),0,HIT(4),0,HIT(4),HIT(5),HIT(4),0,HIT(4),0,HIT(4),0,HIT(4),HIT(5)},
        {HIT(0),0,HIT(4),0,0,0,HIT(4),HIT(5),HIT(0),0,HIT(4),0,0,0,HIT(4),HIT(10)},
        {0,0,0,0,HIT(9),0,0,0,0,0,0,0,HIT(10),0,0,0},
        {0,0,HIT(4),0,HIT(1),0,HIT(4),HIT(5),0,0,HIT(4),0,HIT(1),0,HIT(4),HIT(5)},
        {HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(4),HIT(5),HIT(0)|HIT(4),HIT(4),HIT(4),HIT(4),HIT(1)|HIT(4),HIT(4),HIT(10),HIT(5)},
        {0,0,HIT(4),0,0,0,HIT(4),0,0,0,HIT(4),0,0,0,0,HIT(10)}
    };
    unsigned p;

    for (p = 0u; p < PATTERN_BANK_COUNT; ++p) {
        preset_acid(b, (uint8_t)p, 0u, a_notes[p], a_flags[p]);
        preset_acid(b, (uint8_t)p, 1u, b_notes[p], b_flags[p]);
        preset_drums(b, (uint8_t)p, d808[p], d909[p]);
    }

    /* A few restrained 909 flams in the peak/rebuild parts. */
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
