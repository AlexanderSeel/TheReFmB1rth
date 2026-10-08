// SPDX-License-Identifier: GPL-3.0-only
#include "seq16.h"
#include <string.h>

static uint32_t xorshift32(uint32_t *x){
    uint32_t v=*x?*x:0x6d2b79f5u;
    v^=v<<13; v^=v>>17; v^=v<<5; *x=v; return v;
}

void seq16_init(seq16_t *s,uint32_t seed){
    memset(s,0,sizeof(*s));
    s->rng=seed?seed:1u;
    for(unsigned i=0;i<SEQ16_STEPS;i++) s->step[i].probability=100;
}

void seq16_reset(seq16_t *s){
    s->current=0u;
    s->slide_pending=0u;
}

seq16_event_t seq16_advance(seq16_t *s){
    seq16_event_t e;
    memset(&e,0,sizeof(e));

    const seq16_step_t *st=&s->step[s->current];
    const uint8_t incoming_slide=s->slide_pending;
    s->current=(uint8_t)((s->current+1u)&(SEQ16_STEPS-1u));

    uint8_t p=st->probability>100?100:st->probability;
    if((xorshift32(&s->rng)%100u)>=p){
        s->slide_pending=0u;
        return e;
    }

    if(st->flags&SEQ16_TIE){
        e.valid=1u;
        e.tie=1u;
        e.note_off=0u;
        /* Preserve a source-note slide through a tied duration. */
        return e;
    }

    if(!(st->flags&SEQ16_GATE)){
        s->slide_pending=0u;
        return e;
    }

    e.valid=1u;
    e.note_on=1u;
    e.note=st->note;
    e.accent=(st->flags&SEQ16_ACCENT)!=0u;
    e.slide=incoming_slide;
    s->slide_pending=(st->flags&SEQ16_SLIDE)!=0u;
    e.note_off=!s->slide_pending;
    return e;
}
