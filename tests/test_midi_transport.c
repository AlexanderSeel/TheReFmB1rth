// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/midi_transport.h"

int main(void) {
    midi_transport_t t; midi_transport_init(&t,120u);
    assert(midi_transport_feed_realtime(&t,0xFAu)&MT_EVT_START);
    uint32_t ev=0u;
    for(int i=0;i<6;i++) ev=midi_transport_feed_realtime(&t,0xF8u);
    assert(ev&MT_EVT_STEP);
    assert(midi_transport_feed_realtime(&t,0xFCu)&MT_EVT_STOP);
    assert(!t.playing);
    assert(midi_transport_feed_realtime(&t,0xFBu)&MT_EVT_CONTINUE);
    assert(t.playing);
    midi_transport_t internal; midi_transport_init(&internal,120u); internal.playing=1u;
    ev=midi_transport_process_samples(&internal,5513u);
    assert(ev&MT_EVT_STEP);
    puts("midi transport tests: ok");
    return 0;
}
