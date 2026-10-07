// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/song.h"

int main(void) {
    song_t s; song_init(&s);
    assert(song_append(&s, 2u, 2u));
    assert(song_append(&s, 5u, 1u));
    song_event_t a=song_advance(&s), b=song_advance(&s), c=song_advance(&s);
    assert(a.valid && a.pattern==2u && a.changed);
    assert(b.valid && b.pattern==2u && !b.changed);
    assert(c.valid && c.pattern==5u && c.changed && c.wrapped);
    assert(s.position==0u && s.repeat_index==0u);
    s.loop=0u; song_reset(&s);
    (void)song_advance(&s); (void)song_advance(&s); (void)song_advance(&s);
    assert(!song_advance(&s).valid);
    puts("song tests: ok");
    return 0;
}
