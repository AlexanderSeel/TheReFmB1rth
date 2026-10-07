// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_PROJECT_STORE_H
#define REFM_PROJECT_STORE_H

#include <stddef.h>
#include <stdint.h>

#define PROJECT_STORE_VERSION 1u
#define PROJECT_STORE_HEADER 12u
#define PROJECT_STORE_MAX_PAYLOAD 4096u

enum {
    PROJECT_OK = 0,
    PROJECT_ERR_ARG = -1,
    PROJECT_ERR_SPACE = -2,
    PROJECT_ERR_MAGIC = -3,
    PROJECT_ERR_VERSION = -4,
    PROJECT_ERR_LENGTH = -5,
    PROJECT_ERR_CRC = -6
};

uint32_t project_crc32(const uint8_t *data, size_t len);
int project_encode(uint8_t *out, size_t capacity, const uint8_t *payload, size_t payload_len, size_t *written);
int project_decode(const uint8_t *blob, size_t blob_len, uint8_t *payload, size_t payload_capacity, size_t *payload_len);

#endif
