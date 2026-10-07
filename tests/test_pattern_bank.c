#include <assert.h>
#include <stdio.h>
#include "../firmware/proto/pattern_bank.h"
int main(void){pattern_bank_t b;pattern_bank_init(&b);for(unsigned p=0;p<PATTERN_BANK_COUNT;p++)for(unsigned s=0;s<16u;s++)assert(b.acid[0][p][s].probability==100u);b.acid[0][0][3].note=48u;b.acid[0][0][3].flags=SEQ16_GATE;b.drum_hits[1][0][4]=3u;pattern_bank_copy(&b,1u,0u);assert(b.acid[0][1][3].note==48u&&b.drum_hits[1][1][4]==3u);pattern_bank_clear(&b,1u);assert(b.acid[0][1][3].note==0u&&b.acid[0][1][3].probability==100u);puts("pattern bank tests: ok");return 0;}
