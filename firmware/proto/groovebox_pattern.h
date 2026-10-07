// SPDX-License-Identifier: GPL-3.0-only
#ifndef REFM_GROOVEBOX_PATTERN_H
#define REFM_GROOVEBOX_PATTERN_H
#include <stdint.h>
#include "groovebox.h"
#include "pattern_bank.h"
void groovebox_pattern_store(pattern_bank_t*bank,const groovebox_t*g,uint8_t pattern);void groovebox_pattern_load(const pattern_bank_t*bank,groovebox_t*g,uint8_t pattern);
#endif
