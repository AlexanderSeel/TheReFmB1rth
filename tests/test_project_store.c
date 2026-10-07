// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/proto/project_store.h"

int main(void) {
    const uint8_t src[] = {1u,2u,3u,4u,5u,6u,7u};
    uint8_t blob[128], out[128]; size_t written=0, out_len=0;
    assert(project_encode(blob,sizeof(blob),src,sizeof(src),&written)==PROJECT_OK);
    assert(project_decode(blob,written,out,sizeof(out),&out_len)==PROJECT_OK);
    assert(out_len==sizeof(src) && memcmp(src,out,sizeof(src))==0);
    blob[PROJECT_STORE_HEADER+2u]^=0x55u;
    assert(project_decode(blob,written,out,sizeof(out),&out_len)==PROJECT_ERR_CRC);
    blob[PROJECT_STORE_HEADER+2u]^=0x55u;
    blob[4u]=2u;
    assert(project_decode(blob,written,out,sizeof(out),&out_len)==PROJECT_ERR_VERSION);
    puts("project store tests: ok");
    return 0;
}
