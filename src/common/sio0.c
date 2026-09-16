/*
 * ps1-bare-metal - (C) 2023-2025 spicyjpeg
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "common/sio0.h"
#include "ps1/delay.h"
#include "ps1/registers.h"

void initControllerBus(void) {
	SIO_CR(0) = SIO_CR_INTRST;

	SIO_MR(0) = 0
		| SIO_MR_BR_DIV1
		| SIO_MR_CHLEN_8;
	SIO_BR(0) = F_CPU / 250000;
	SIO_CR(0) = 0
		| SIO_CR_TXEN
		| SIO_CR_RXEN
		| SIO_CR_DSRIEN;

	IRQ_MASK &= ~(1 << IRQ_SIO0);
}

static bool waitForAcknowledge(int timeout) {
	for (; timeout > 0; timeout -= 10) {
		if (IRQ_STAT & (1 << IRQ_SIO0)) {
			IRQ_STAT   = ~(1 << IRQ_SIO0);
			SIO_CR(0) |= SIO_CR_ERRRST;

			return true;
		}

		delayMicroseconds(10);
	}

	return false;
}

#define DTR_DELAY    60
#define DSR_TIMEOUT 120

void selectControllerPort(int port) {
	uint16_t cr = port ? SIO_CR_PORT_2 : SIO_CR_PORT_1;
	SIO_CR(0)   = (SIO_CR(0) & ~SIO_CR_PORT_BITMASK) | cr;
}

static uint8_t exchangeByte(uint8_t value) {
	while (!(SIO_SR(0) & SIO_SR_TXRDY))
		__asm__ volatile("");

	SIO_DR(0) = value;

	while (!(SIO_SR(0) & SIO_SR_RXRDY))
		__asm__ volatile("");

	return SIO_DR(0);
}

size_t exchangeSIO0Packet(
	SIO0DeviceAddress address,
	const uint8_t     *request,
	uint8_t           *response,
	size_t            reqLength,
	size_t            maxRespLength
) {
	IRQ_STAT   = ~(1 << IRQ_SIO0);
	SIO_CR(0) |= SIO_CR_DTR | SIO_CR_ERRRST;
	delayMicroseconds(DTR_DELAY);

	size_t respLength = 0;

	SIO_DR(0) = address;

	if (waitForAcknowledge(DSR_TIMEOUT)) {
		while (SIO_SR(0) & SIO_SR_RXRDY)
			SIO_DR(0);

		while (respLength < maxRespLength) {
			if (reqLength > 0) {
				*(response++) = exchangeByte(*(request++));
				reqLength--;
			} else {
				*(response++) = exchangeByte(0);
			}

			respLength++;

			// FIXME: this is not 100% reliable due to metastability issues...
			if (!waitForAcknowledge(DSR_TIMEOUT))
				break;
		}
	}

	delayMicroseconds(DTR_DELAY);
	SIO_CR(0) &= ~SIO_CR_DTR;

	return respLength;
}

uint16_t pollController(int port) {
	uint8_t request[4], response[8];

	request[0] = SIO0_PAD_POLL; // Command
	request[1] = 0x00;          // Multitap address
	request[2] = 0x00;          // Actuator control 1
	request[3] = 0x00;          // Actuator control 2

	selectControllerPort(port);
	size_t respLength = exchangeSIO0Packet(
		SIO0_ADDR_CONTROLLER,
		request,
		response,
		sizeof(request),
		sizeof(response)
	);

	if (respLength < 4)
		return 0;
	if ((response[0] >> 4) == PAD_TYPE_NONE)
		return 0;

	return (response[2] | (response[3] << 8)) ^ 0xffff;
}
