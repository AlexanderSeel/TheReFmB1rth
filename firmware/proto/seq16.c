// SPDX-License-Identifier: GPL-3.0-only
#include "seq16.h"
#include <string.h>
static uint32_t xorshift32(uint32_t *x){ uint32_t v=*x?*x:0x6d2b79f5u; v^=v<<13; v^=v>>17; v^=v<<5; *x=v; return v; }
void seq16_init(seq16_t *s,uint32_t seed){ memset(s,0,sizeof(*s)); s->rng=seed?seed:1u; for(unsigned i=0;i<SEQ16_STEPS;i++) s->step[i].probability=100; }
void seq16_reset(seq16_t *s){ s->current=0; }
seq16_event_t seq16_advance(seq16_t *s){ seq16_event_t e; memset(&e,0,sizeof(e)); const seq16_step_t *st=&s->step[s->current]; s->current=(uint8_t)((s->current+1u)&(SEQ16_STEPS-1u)); uint8_t p=st->probability>100?100:st->probability; if(!(st->flags&(SEQ16_GATE|SEQ16_TIE))) return e; if((xorshift32(&s->rng)%100u)>=p) return e; e.valid=1; e.note=st->note; e.accent=(st->flags&SEQ16_ACCENT)!=0; e.slide=(st->flags&SEQ16_SLIDE)!=0; e.tie=(st->flags&SEQ16_TIE)!=0; e.note_on=(st->flags&SEQ16_GATE)!=0; e.note_off=!e.tie&&!e.slide; return e; }
