// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/proto/sample_voice.h"

int main(void) {
    static const int16_t pcm[] = {0, 12000, -12000, 24000, 0};
    sample_voice_t v;
    sample_voice_init(&v);
    assert(sample_voice_process(&v) == 0);

    sample_voice_trigger(&v, pcm, 5u, 127u);
    assert(v.active);
    assert(sample_voice_process(&v) == 0);
    assert(sample_voice_process(&v) > 11000);
    assert(sample_voice_process(&v) < -11000);
    sample_voice_process(&v);
    sample_voice_process(&v);
    assert(!v.active);
    assert(sample_voice_process(&v) == 0);

    sample_voice_trigger_rate(&v, pcm, 5u, 64u, 32768u); /* half-speed, interpolated */
    assert(sample_voice_process(&v) == 0);
    {
        int16_t mid = sample_voice_process(&v);
        assert(mid > 2500 && mid < 3500); /* ~6000 * 64/127 */
    }

    sample_voice_trigger(&v, pcm, 5u, 0u);
    assert(!v.active);
    puts("sample voice tests: ok");
    return 0;
}
