/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - TWI driver
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

#ifdef	MT_HW_TWI

volatile mt_twi_state_t mt_twi_state;

/* initialize TWI driver */
void mt_twi_init(void) {
	/* configure port */
	PORTC |= _BV(PC4) | _BV(PC5);
	DDRC &= ~(_BV(PC4) | _BV(PC5));

	/* prescaler 1 */
	TWSR = 0 << TWPS0;
	TWBR = ((F_CPU / 1) / 100000 - 16) / 2;
	TWCR = 0;

	mt_twi_state.address = 0x40;
	mt_twi_state.status = 1;
	mt_twi_state.value = 0;
	mt_twi_state.current = 0xff;
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
}

/* update port value
 *
 * value		new port value
 */
void mt_twi_update(uint8_t value) {
	uint8_t tmp;

	/* lock interrupts to exclusively have the buffer */
	tmp = SREG;
	cli();

	/* write data to state */
	if (mt_twi_state.value != value) {
		mt_twi_state.value = value;
		/* check if we need to start transmission */
		if (mt_twi_state.status == 0) {
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
			mt_twi_state.status = 1;
		}
	}

	/* restore interrupt status */
	SREG = tmp;
}

/* TWI state change ISR */
ISR(TWI_vect) {
	uint8_t error;

	/* check if TWI driver is active */
	if (mt_twi_state.status == 0)
		return;

	/* handle TWI status */
	error = 0;
	switch (TW_STATUS) {
	case TW_BUS_ERROR:
		/* handle TWI bus error */
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO);
		error = 1;
		break;
	case TW_START:
	case TW_REP_START:
		/* START or REPEATED START sent, send SLA+R/W */
		TWDR = mt_twi_state.address;
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
		break;
	case TW_MT_SLA_ACK:
		/* SLA+W successfully sent, send value */
		mt_twi_state.sending = mt_twi_state.value;
		TWDR = mt_twi_state.value;
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
		break;
	case TW_MT_DATA_ACK:
		/* data successfully sent, finish transaction */
		mt_twi_state.current = mt_twi_state.sending;
		mt_twi_state.status = 0;
		break;
	case TW_MT_SLA_NACK:
		/* SLA+W sent, but no ACK, abort transmission */
	case TW_MT_DATA_NACK:
		/* data sent, but no ACK, abort transmission */
		mt_twi_state.status = 0;
		error = 1;
		break;
	case TW_MT_ARB_LOST:
		/* arbitration lost, just retry */
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
		break;
	case TW_MR_SLA_ACK:
		/* SLA+R successfully sent, get data byte (no ACK) */
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
		break;
	case TW_MR_SLA_NACK:
		/* SLA+R sent, but no ACK, abort transmission */
		mt_twi_state.status = 0;
		error = 1;
		break;
	case TW_MR_DATA_ACK:
	case TW_MR_DATA_NACK:
		/* data received, store value */
		mt_twi_state.current = TWDR;
		mt_twi_state.status = 0;
		break;
	case TW_SR_SLA_ACK:
	case TW_SR_ARB_LOST_SLA_ACK:
	case TW_SR_GCALL_ACK:
	case TW_SR_ARB_LOST_GCALL_ACK:
	case TW_SR_DATA_ACK:
	case TW_SR_DATA_NACK:
	case TW_SR_GCALL_DATA_ACK:
	case TW_SR_GCALL_DATA_NACK:
	case TW_SR_STOP:
	case TW_ST_SLA_ACK:
	case TW_ST_ARB_LOST_SLA_ACK:
	case TW_ST_DATA_ACK:
	case TW_ST_DATA_NACK:
	case TW_ST_LAST_DATA:
		/* slave mode not implemented */
	case TW_NO_INFO:
		/* nothing to do if no info is available */
	default:
		/* generic fallthrough if no valid status found */
		break;
	}

	/* check if bus is idle and another update is pending */
	if (mt_twi_state.status == 0) {
		if (mt_twi_state.value != mt_twi_state.current && error == 0) {
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA);
			mt_twi_state.status = 1;
		} else {
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO);
		}
	}
}
#endif	/* MT_HW_TWI */
