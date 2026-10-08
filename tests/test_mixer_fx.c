// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/mixer_fx.h"

int main(void) {
    mixer_fx_t m; mixer_fx_init(&m);
    int16_t in[MIX_TRACKS]={12000,8000,4000,2000},l=0,r=0;
    mixer_fx_process(&m,in,&l,&r);
    assert(l!=0 && r!=0);

    mixer_fx_init(&m);
    m.track[0].mute=1u; m.track[1].mute=1u; m.track[2].mute=1u; m.track[3].mute=1u;
    mixer_fx_process(&m,in,&l,&r);
    assert(l==0 && r==0);

    mixer_fx_init(&m); m.track[2].solo=1u;
    int16_t only[MIX_TRACKS]={20000,20000,5000,20000};
    mixer_fx_process(&m,only,&l,&r);
    assert(l>0 && l<12000 && r>0 && r<12000);

    mixer_fx_init(&m); mixer_fx_set_delay(&m,2u,12000,20000); m.track[0].delay_send=127u;
    int16_t impulse[MIX_TRACKS]={20000,0,0,0}; mixer_fx_process(&m,impulse,&l,&r);
    int16_t zero[MIX_TRACKS]={0,0,0,0}; mixer_fx_process(&m,zero,&l,&r); mixer_fx_process(&m,zero,&l,&r);
    assert(l!=0 || r!=0);

    /* Project files are CRC protected but an older/foreign valid project can
       still carry an out-of-range delay length. The audio path must never use
       that value as an unchecked array bound. */
    mixer_fx_init(&m);
    m.delay_len = (uint16_t)(MIX_DELAY_MAX + 157u);
    m.delay_pos = (uint16_t)(MIX_DELAY_MAX + 20u);
    mixer_fx_process(&m,in,&l,&r);
    assert(m.delay_len == MIX_DELAY_MAX);
    assert(m.delay_pos < MIX_DELAY_MAX);

    mixer_fx_init(&m);
    m.delay_len = 0u;
    m.delay_pos = 99u;
    mixer_fx_process(&m,in,&l,&r);
    assert(m.delay_len == 1u);
    assert(m.delay_pos == 0u);

    puts("mixer/fx tests: ok");
    return 0;
}
