// SPDX-License-Identifier: GPL-3.0-only
#include "project_store.h"
#include <string.h>

static const uint8_t MAGIC[4] = {'R','F','M','1'};

static void put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void put32(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1]<<8); }
static uint32_t get32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }

uint32_t project_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i=0; i<len; ++i) {
        crc ^= data[i];
        for (unsigned b=0; b<8u; ++b) crc = (crc>>1) ^ (0xEDB88320u & (0u-(crc&1u)));
    }
    return ~crc;
}

int project_encode(uint8_t *out, size_t capacity, const uint8_t *payload, size_t payload_len, size_t *written) {
    if (!out || !payload || !written) return PROJECT_ERR_ARG;
    if (payload_len > PROJECT_STORE_MAX_PAYLOAD || payload_len > 0xFFFFu) return PROJECT_ERR_LENGTH;
    if (capacity < PROJECT_STORE_HEADER + payload_len) return PROJECT_ERR_SPACE;
    memcpy(out, MAGIC, 4u);
    put16(out+4u, PROJECT_STORE_VERSION);
    put16(out+6u, (uint16_t)payload_len);
    put32(out+8u, project_crc32(payload, payload_len));
    memcpy(out+PROJECT_STORE_HEADER, payload, payload_len);
    *written = PROJECT_STORE_HEADER + payload_len;
    return PROJECT_OK;
}

int project_decode(const uint8_t *blob, size_t blob_len, uint8_t *payload, size_t payload_capacity, size_t *payload_len) {
    if (!blob || !payload || !payload_len) return PROJECT_ERR_ARG;
    if (blob_len < PROJECT_STORE_HEADER) return PROJECT_ERR_LENGTH;
    if (memcmp(blob, MAGIC, 4u) != 0) return PROJECT_ERR_MAGIC;
    if (get16(blob+4u) != PROJECT_STORE_VERSION) return PROJECT_ERR_VERSION;
    size_t len = get16(blob+6u);
    if (len > PROJECT_STORE_MAX_PAYLOAD || PROJECT_STORE_HEADER + len != blob_len) return PROJECT_ERR_LENGTH;
    if (payload_capacity < len) return PROJECT_ERR_SPACE;
    if (project_crc32(blob+PROJECT_STORE_HEADER, len) != get32(blob+8u)) return PROJECT_ERR_CRC;
    memcpy(payload, blob+PROJECT_STORE_HEADER, len);
    *payload_len = len;
    return PROJECT_OK;
}
