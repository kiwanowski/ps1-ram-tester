/*
 * ps1-bare-metal - (C) 2023-2026 spicyjpeg
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define SPU_RAM_ALLOC_OFFSET 0x1000

#define SPU_NUM_CHANNELS     24
#define SPU_MAX_VOLUME   0x3fff

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t spuRAMAddressShift;

void initSPU(void);
void waitForSPUDMADone(void);

void sendSPURAMData(const void *data, unsigned int offset, size_t length);
void receiveSPURAMData(void *data, unsigned int offset, size_t length);

void stopAllSPUChannels(void);
int findFreeSPUChannel(void);

#ifdef __cplusplus
}
#endif
