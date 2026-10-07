// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/seq16.h"
int main(void){ seq16_t s; seq16_init(&s,1); s.step[0]=(seq16_step_t){60,SEQ16_GATE|SEQ16_ACCENT,100,0}; s.step[1]=(seq16_step_t){62,SEQ16_GATE|SEQ16_SLIDE,100,0}; s.step[2]=(seq16_step_t){0,SEQ16_TIE,100,0}; seq16_event_t a=seq16_advance(&s),b=seq16_advance(&s),c=seq16_advance(&s); assert(a.valid&&a.note_on&&a.accent&&a.note_off); assert(b.valid&&b.slide&&!b.note_off); assert(c.valid&&c.tie&&!c.note_on&&!c.note_off); for(int i=3;i<16;i++)seq16_advance(&s); assert(s.current==0); puts("seq16 tests: ok"); return 0; }
