// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/seq16.h"

int main(void){
    seq16_t s;
    seq16_init(&s,1u);

    /* SLIDE belongs to the source note: C -> D slides because C has SLIDE. */
    s.step[0]=(seq16_step_t){60,SEQ16_GATE|SEQ16_ACCENT|SEQ16_SLIDE,100,0};
    s.step[1]=(seq16_step_t){62,SEQ16_GATE,100,0};
    s.step[2]=(seq16_step_t){0,SEQ16_TIE,100,0};
    s.step[3]=(seq16_step_t){65,SEQ16_GATE,100,0};

    seq16_event_t a=seq16_advance(&s);
    seq16_event_t b=seq16_advance(&s);
    seq16_event_t c=seq16_advance(&s);
    seq16_event_t d=seq16_advance(&s);

    assert(a.valid&&a.note_on&&a.accent&&!a.slide&&!a.note_off);
    assert(b.valid&&b.note_on&&b.slide&&b.note==62u&&b.note_off);
    assert(c.valid&&c.tie&&!c.note_on&&!c.note_off);
    assert(d.valid&&d.note_on&&!d.slide&&d.note==65u);

    /* A source slide survives a TIE and targets the next actual note. */
    seq16_reset(&s);
    s.step[0]=(seq16_step_t){48,SEQ16_GATE|SEQ16_SLIDE,100,0};
    s.step[1]=(seq16_step_t){0,SEQ16_TIE,100,0};
    s.step[2]=(seq16_step_t){55,SEQ16_GATE,100,0};
    a=seq16_advance(&s); b=seq16_advance(&s); c=seq16_advance(&s);
    assert(a.valid&&!a.slide);
    assert(b.valid&&b.tie);
    assert(c.valid&&c.slide&&c.note==55u);

    /* REST clears any pending slide. */
    seq16_reset(&s);
    s.step[0]=(seq16_step_t){48,SEQ16_GATE|SEQ16_SLIDE,100,0};
    s.step[1]=(seq16_step_t){0,0,100,0};
    s.step[2]=(seq16_step_t){55,SEQ16_GATE,100,0};
    (void)seq16_advance(&s);
    b=seq16_advance(&s); c=seq16_advance(&s);
    assert(!b.valid);
    assert(c.valid&&!c.slide);

    for(int i=3;i<16;i++) (void)seq16_advance(&s);
    assert(s.current==0u);
    puts("seq16 tests: ok");
    return 0;
}
