// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/mixer_fx.h"

int main(void) {
    mixer_fx_t m; mixer_fx_init(&m);

    /* Fresh firmware is a clean mixer: unity track faders, no nonlinear master
       processing and a fully open PCF. Summing headroom is owned by the master
       bus rather than hidden in individual tracks. */
    for(unsigned i=0;i<MIX_TRACKS;++i) assert(m.track[i].level == 32767);
    assert(m.drive == 0);
    assert(m.compressor_threshold == 32767);
    assert(m.filter_cutoff == 32767);

    /* A single track is a linear -12 dB path at startup. */
    int16_t one[MIX_TRACKS]={16000,0,0,0},l=0,r=0;
    mixer_fx_process(&m,one,&l,&r);
    assert(l>=3950 && l<=4050);
    assert(r>=3950 && r<=4050);

    /* Four simultaneously hot tracks must still not hit a numerical rail with
       DIST and COMP off. This is the hardware regression for beta.7's globally
       overdriven sound. */
    mixer_fx_init(&m);
    int16_t hot[MIX_TRACKS]={30000,30000,30000,30000};
    mixer_fx_process(&m,hot,&l,&r);
    assert(l>29000 && l<31000);
    assert(r>29000 && r<31000);
    assert(l!=32767 && l!=-32768 && r!=32767 && r!=-32768);

    int16_t in[MIX_TRACKS]={12000,8000,4000,2000};
    mixer_fx_init(&m); mixer_fx_process(&m,in,&l,&r);
    assert(l!=0 && r!=0);

    mixer_fx_init(&m);
    m.track[0].mute=1u; m.track[1].mute=1u; m.track[2].mute=1u; m.track[3].mute=1u;
    mixer_fx_process(&m,in,&l,&r);
    assert(l==0 && r==0);

    mixer_fx_init(&m); m.track[2].solo=1u;
    int16_t only[MIX_TRACKS]={20000,20000,5000,20000};
    mixer_fx_process(&m,only,&l,&r);
    assert(l>1000 && l<1500 && r>1000 && r<1500);

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
