/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - serial interface driver
 * Copyright (C) 2009-2011 Michael Kukat <michael_AT_mik-music.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miditriggs.h"

/* ring buffer data structure */
typedef struct ringbuffer {
	uint8_t size;
	uint8_t inptr;
	uint8_t outptr;
	uint8_t data[];
} ringbuffer_t;

/* heap for all buffers */
#define	MTMISIZE	(sizeof(struct ringbuffer) * 2 + MT_TXBSIZE + MT_RXBSIZE)
uint8_t mt_midi_heap[MTMISIZE];

static struct ringbuffer *rxbuffer;
static struct ringbuffer *txbuffer;

/* USART baud rate register value */
#define	UBAUD	(F_CPU / 16 / MIDI_BPS - 1)

/* put a data byte to the ring buffer (lowlevel)
 *
 * ringbuffer	The ring buffer to put the byte into
 * data			Data byte to put to the ring buffer
 *
 * return		1 if buffer is full, 0 on success
 */
static inline uint8_t _ringbuffer_put(ringbuffer_t *ringbuffer, uint8_t data) {
	uint8_t size, inptr, outptr;

	/* put data to ring buffer and increment input pointer */
	inptr = ringbuffer->inptr;
	ringbuffer->data[inptr++] = data;
	size = ringbuffer->size;
	if (size != 0 && inptr >= size)
		inptr = 0;

	/* check if buffer is full, return 1 is true */
	outptr = ringbuffer->outptr;
	if (inptr == outptr) {
		return 1;
	}

	/* update input pointer and return */
	ringbuffer->inptr = inptr;
	return 0;
}

/* get a data byte from the ring buffer (lowlevel)
 *
 * ringbuffer	The ring buffer to look for a byte
 *
 * return		The data byte or -1 if buffer is empty
 */
static inline int16_t _ringbuffer_get(ringbuffer_t *ringbuffer) {
	uint8_t size, inptr, outptr, data;

	/* check if data is available, return -1 if not */
	inptr = ringbuffer->inptr;
	outptr = ringbuffer->outptr;
	if (inptr == outptr) {
		return -1;
	}

	/* get data from ring buffer and increment output pointer */
	data = ringbuffer->data[outptr++];
	size = ringbuffer->size;
	if (size != 0 && outptr >= size)
		outptr = 0;
	ringbuffer->outptr = outptr;

	/* and return */
	return data;
}

/* initialize ring buffer with given size
 *
 * ringbuffer	The ring buffer to initialize
 * size			The size of the ring buffer data area
 */
void ringbuffer_init(ringbuffer_t *ringbuffer, uint8_t size) {
	uint8_t tmp;

	/* lock interrupts to exclusively have the pointers */
	tmp = SREG;
	cli();

	/* reset size and pointers */
	ringbuffer->size = size;
	ringbuffer->inptr = 0;
	ringbuffer->outptr = 0;

	/* restore interrupt status */
	SREG = tmp;
}

/* initialize serial interfaces */
void mt_serial_init(void) {
	uint8_t *p;

	/* get pointer to local heap */
	p = mt_midi_heap;

	/* calculate ring buffer area pointers and initialize ring buffers */
	txbuffer = (struct ringbuffer *) p;
	ringbuffer_init(txbuffer, MT_TXBSIZE);
	p += sizeof(struct ringbuffer) + MT_TXBSIZE;

	rxbuffer = (struct ringbuffer *) p;
	ringbuffer_init(rxbuffer, MT_RXBSIZE);
	p += sizeof(struct ringbuffer) + MT_RXBSIZE;

	/* initialize USART for given MCU type */
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega644__)
	UBRR0 = UBAUD;
	UCSR0A = (1 << TXC0);
	UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
	UCSR0C = (3 << UCSZ00) | (0 << UPM00) | (0 << USBS0);
#elif defined(__AVR_ATmega32__)
	UBRRH = UBAUD >> 8;
	UBRRL = UBAUD & 0xff;
	UCSRA = (1 << TXC);
	UCSRB = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);
	UCSRC = (3 << UCSZ0) | (0 << UPM0) | (0 << USBS) | (1 << URSEL);
#else
#error "unsupported MCU type"
#endif
}

/* get a byte from the ring buffer
 *
 * return		The data byte or -1 if buffer is empty
 */
int16_t mt_serial_get(void) {
	int16_t tmp, ret;

	/* lock interrupts */
	tmp = SREG;
	cli();

	/* try to get a byte from the buffer first */
	ret = _ringbuffer_get(rxbuffer);

	/* restore interrupt status and return status */
	SREG = tmp;
	return ret;
}

/* put a data byte to ring buffer
 *
 * data			The data byte to put to the buffer
 *
 * return		1 if buffer is full, 0 on success
 */
uint8_t mt_serial_put(uint8_t data) {
	uint8_t tmp, ret;

	/* lock interrupts */
	tmp = SREG;
	cli();

	/* put data to buffer */
	ret = _ringbuffer_put(txbuffer, data);

	/* make sure UDR empty interrupt is enabled */
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega644__)
	if (!(UCSR0B & _BV(UDRIE0)))
	UCSR0B |= _BV(UDRIE0);
#elif defined(__AVR_ATmega32__)
	if(!(UCSRB & _BV(UDRIE))) UCSRB |= _BV(UDRIE);
#else
#error "unsupported MCU type"
#endif

	/* restore interrupt status and return status */
	SREG = tmp;
	return ret;
}

/* USART receive ISR */
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__)
ISR(USART_RX_vect)
#elif defined(__AVR_ATmega644__)
ISR(USART0_RX_vect)
#elif defined(__AVR_ATmega32__)
ISR(USART_RXC_vect)
#else
#error "unsupported MCU type"
void txdummy(void) /* makes eclipse happy */
#endif
{
	uint8_t data;

	/* get data byte and try to put it to the buffer */
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega644__)
	data = UDR0;
#elif defined(__AVR_ATmega32__)
	data = UDR;
#else
#error "unsupported MCU type"
#endif
	_ringbuffer_put(rxbuffer, data);
}

/* USART transmit ISR */
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega32__)
ISR(USART_UDRE_vect)
#elif defined(__AVR_ATmega644__)
ISR(USART0_UDRE_vect)
#else
#error "unsupported MCU type"
void rxdummy(void) /* makes eclipse happy */
#endif
{
	int16_t data;

	/* check if more data bytes for transmission are available */
	data = _ringbuffer_get(txbuffer);

	/* send data byte if available, otherwise, set transmitter to inactive */
#if defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega644__)
	if (data >= 0)
	UDR0 = data;
	else
	UCSR0B &= ~_BV(UDRIE0);
#elif defined(__AVR_ATmega32__)
	if(data >= 0) UDR = data;
	else UCSRB &= ~_BV(UDRIE);
#else
#error "unsupported MCU type"
#endif
}
