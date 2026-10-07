// SPDX-License-Identifier: GPL-3.0-only
#include "groovebox_pattern.h"
#include <string.h>
void groovebox_pattern_store(pattern_bank_t*b,const groovebox_t*g,uint8_t p){if(p>=PATTERN_BANK_COUNT)return;for(unsigned a=0;a<2u;a++)memcpy(b->acid[a][p],g->acid_seq[a].step,sizeof(b->acid[a][p]));for(unsigned d=0;d<2u;d++){memcpy(b->drum_hits[d][p],g->drum_hits[d],sizeof(b->drum_hits[d][p]));memcpy(b->drum_accents[d][p],g->drum_accents[d],sizeof(b->drum_accents[d][p]));}}
void groovebox_pattern_load(const pattern_bank_t*b,groovebox_t*g,uint8_t p){if(p>=PATTERN_BANK_COUNT)return;for(unsigned a=0;a<2u;a++){memcpy(g->acid_seq[a].step,b->acid[a][p],sizeof(b->acid[a][p]));seq16_reset(&g->acid_seq[a]);}for(unsigned d=0;d<2u;d++){memcpy(g->drum_hits[d],b->drum_hits[d][p],sizeof(b->drum_hits[d][p]));memcpy(g->drum_accents[d],b->drum_accents[d][p],sizeof(b->drum_accents[d][p]));}g->current_pattern=p;g->step=0u;}
