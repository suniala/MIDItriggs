/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - global definitions
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
#ifndef MIDITRIGGS_H_
#define MIDITRIGGS_H_

#include <inttypes.h>
#ifdef	__AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/twi.h>
#define wdr() asm volatile("wdr");
#else	/* __AVR__ */
#include <stdio.h>
#define	PROGMEM
#define	pgm_read_byte(a) (*(a))
#define	pgm_read_word(a) (*(a))
#define	eeprom_read_byte(a) (0)
#define	eeprom_read_word(a) (0)
#define	eeprom_read_dword(a) (0)
#define	eeprom_write_byte(a, b)
#define wdr()
#endif	/* __AVR__ */

/* target and features configuration */
#define	MT_HW_USESEQ						/* Use machine sequencer */
#define	MT_HW_CFGBUTTON						/* Use single-button-configuration */
#define	MT_HW_CFGSEQRUN						/* configuration needs running hardware sequencer */
#undef	MT_HW_CFGENCDSP						/* Use encoder and display for configuration */
#define	MT_HW_TWI							/* include TWI driver */
#define	MT_HW_PGMCHG						/* include program change function */

/* for obvious reasons, you may only define one target hardware */
#undef	MT_HW_HARP							/* Target hardware is original HARP proto board */
#undef	MT_HW_HARP2							/* Target hardware is the universal board for HARP */
#define	MT_HW_CR78							/* universal board (HARP2) in Roland CR-78 */

/* system configuration */
#define	MT_VER_MAJ		1					/* MIDItriggs major version */
#define	MT_VER_MIN		0					/* MIDItriggs minor version */
#define	MT_MAXTRIG		16					/* maximum number of triggers */
#define	MT_VTRIGGERS	2					/* virtual triggers */
#define	MT_TXBSIZE		64					/* transmit buffer size */
#define	MT_RXBSIZE		128					/* receive buffer size */
#define	MT_BTIME		1000				/* minimum time for button long push */
#define	MT_SEQTIME		250					/* sequencer step time */

/* generic defines */
#define	MIDI_BPS		31250				/* MIDI baud rate */
#define	CRC8_POLY		0x07				/* default CRC8 polynom x^8 + x^2 + x^1 + 1 */
#define	CRC8_INIT		0x00				/* CRC8 initial value */
#define	EEP_RESVD		28					/* 28 bytes EEPROM reserved for bootloader */
#define	EEP_DATA		(512 - EEP_RESVD)	/* configuration data area in EEPROM */

/* target specific stuff */
#include "mt_harp.h"
#include "mt_cr78.h"

/* create version strings */
#define	MT_STR(x)	MT_STR2(x)
#define	MT_STR2(x)	#x
#define	MT_VERSTR	MT_STR(MT_VER_MAJ.MT_VER_MIN)
#if MT_VER_MAJ < 10
#define	MT_VER_P1	"V"
#else
#define	MT_VER_P1	""
#endif
#if MT_VER_MIN < 10
#define	MT_VER_P2	" "
#else
#define	MT_VER_P2	""
#endif
#define	MT_GREETVER	MT_VER_P2 MT_VER_P1 MT_VERSTR

/* configuration data structures */
typedef struct boot_eeprom {
	uint8_t signature[4];					/* signature (see above) */
	uint16_t hwtype;						/* hardware type (16 bit) */
	uint16_t hwvers;						/* hardware version (4-digit BCD) */
	uint32_t serial;						/* serial number (32bit unsigned int) */
	uint32_t build;							/* build date (8-digit BCD) */
	uint16_t romvers;						/* boot rom version (4-digit BCD) */
	uint8_t midimdl[2];						/* MIDI model (7 bits per byte) */
	uint8_t midimfr[3];						/* MIDI manufacturer (7 bits per byte) */
	uint8_t actmethod;						/* boot loader activation method */
	uint8_t reserved[3];					/* for future use */
	uint8_t cksum;							/* CRC-8 over header data */
}__attribute((packed)) mt_boot_eeprom_t;

typedef struct mt_map_inst_in {
	uint8_t trigger;						/* trigger number */
	uint8_t triglen;						/* trigger length */
}__attribute((packed)) mt_map_inst_in_t;

typedef struct mt_map_inst_out {
	uint8_t note;							/* note value */
	uint8_t channel;						/* MIDI channel (0xff if global out) */
	uint8_t velocity;						/* velocity value */
	uint8_t length;							/* note length, currently unused */
	uint8_t name[8];						/* instrument name */
}__attribute((packed)) mt_map_inst_out_t;

typedef struct mt_config_data {
	uint16_t midi_in_channel :4;				/* MIDI receive channel */
	uint16_t midi_out_channel :4;			/* MIDI transmit channel */
	uint16_t midi_in_omni :1;				/* MIDI receive omni mode enabled (1) */
	uint16_t midi_in_sync :1;				/* sync to received MIDI sync (1) */
	uint16_t midi_out_use_in :1;				/* use receive channel for transmit */
	uint16_t midi_out_enable :1;				/* enable MIDI out features */
	uint16_t midi_out_sync :1;				/* output clock/start/stop messages */
	uint16_t display_line1 :1;				/* display line 1, 0 = status, 1 = static message */
	uint16_t display_line2 :1;				/* display line 2, 0 = triggers, 1 = static message */
	uint16_t midi_out_unused :1;				/* for future use */
	mt_map_inst_in_t inst_in[128];			/* incoming instrument mapping (note->trigger) */
	mt_map_inst_out_t inst_out[MT_MAXTRIG];	/* outgoing instrument mapping (trigger->note) */
	uint8_t display_text[2][16];			/* generic display text */
	uint8_t out_bpm;						/* speed of internal sync generator */
	uint8_t crc;							/* CRC-8 over configuration structure */
}__attribute__((packed)) mt_config_data_t;

/* The sync state engine works the following way:
 *
 * Upon power up and in idle mode, MT_SYNC_OFF is set.
 * When the machine sequencer is manually started, MT_SYNC_MACH_START is set.
 * The transition from MT_SYNC_MACH_START to MT_SYNC_MACH_RUN is done by the
 * MIDI transmission function as soon as MIDI start is sent. This also has to
 * be kept intact if MIDI sync out is turned off, the transition then has to
 * be done without sending the start message.
 * When the machine sequencer is stopped, MT_SYNC_MACH_STOP is set, which is
 * used for the MIDI stack to send the stop event and put the unit in
 * MT_SYNC_OFF state afterwards. The same rules as above apply here, if MIDI
 * sync out is turned off.
 * From MT_SYNC_OFF, a transition to MT_SYNC_MIDI_START is done by receiving
 * a MIDI start message. The first clock after the start message sets the unit
 * to MT_SYNC_MIDI_RUN state, until a MIDI stop message comes, which sets the
 * MT_SYNC_MIDI_STOP state. The hardware now has the possibility to either go
 * to the MT_SYNC_MIDI_FREE state to keep alive a fade-out using the internal
 * sync timer to generate the clocks or the MT_SYNC_OFF state if the stop
 * request completed processing, while this state can be entered by hardware
 * after a fade-out running during MT_SYNC_MIDI_FREE.
 * The special state MT_SYNC_BLOCKED is there to provide some sort of special
 * state to disable automatic clocking for some purpose.
 */
typedef enum {
	MT_SYNC_OFF = 0,						/* clocks stopped */
	MT_SYNC_MACH_START,						/* machine has been started */
	MT_SYNC_MACH_RUN,						/* machine is running */
	MT_SYNC_MACH_STOP,						/* machine has been stopped */
	MT_SYNC_MIDI_START,						/* start via MIDI */
	MT_SYNC_MIDI_RUN,						/* MIDI clock is running */
	MT_SYNC_MIDI_STOP,						/* stop via MIDI */
	MT_SYNC_MIDI_FREE,						/* free running from MIDI-derived clock */
	MT_SYNC_BLOCKED							/* unit is blocked */
} mt_syncmode_t;
extern volatile mt_syncmode_t mt_syncmode;	/* synchronization mode */

/* TWI driver state */
typedef struct mt_twi_state {
	uint16_t address :7;						/* address of device to write data to */
	uint16_t status :1;						/* status, 0 = idle, 1 = active */
	uint8_t value;							/* value to send to port */
	uint8_t sending;						/* current value in transmission */
	uint8_t current;						/* current value on port */
}__attribute__((packed)) mt_twi_state_t;
extern volatile mt_twi_state_t mt_twi_state;

/* global variables/functions */
extern volatile uint16_t mt_timer;				/* generic timer, incremented in 1ms intervals */
extern volatile uint16_t mt_counter;			/* generic counter, bits 8-23 (timer 2 is for LSB) */
extern volatile uint32_t mt_midi_prevcnt;		/* previous counter value to derive MIDI clock timer value */
extern volatile uint32_t mt_midi_timeval;		/* 24 bit timer value for MIDI clock */
extern volatile uint32_t mt_machine_prevcnt;	/* previous counter value to derive machine clock timer value */
extern volatile uint32_t mt_machine_timeval;	/* 24 bit timer value for machine clock */
extern volatile uint32_t mt_prev_timeval;		/* previous timer value to detect jitter */
extern volatile uint8_t mt_synclock;			/* sync lock counter */
extern mt_config_data_t mt_config_data;			/* configuration data */
extern volatile uint8_t mt_midi_clock;			/* MIDI clock send counter */
#ifdef	MT_HW_PGMCHG
/* current program number */
extern uint8_t mt_midi_program;					/* MIDI program number */
#endif	/* MT_HW_PGMCHG */

/* trigger counters/flags */
#define	MT_TRIG_EXT		0x80
#define	MT_TRIG_SEEN	0x40
extern volatile uint8_t mt_triggers[MT_TRIGGERS + MT_VTRIGGERS];	/* trigger in/out status */
extern volatile uint8_t mt_triglatch[MT_TRIGGERS];	/* trigger latches for synchronized triggers */
extern volatile uint8_t mt_latchdelay;		/* latch delay counter for trigger sync */
extern uint16_t mt_trigsum;					/* trigger summary for display output */

/* static inline functions to include in ISRs
 *
 * return		the correct 32 bit counter value
 */
static inline uint32_t mt_get_counter(void) {
#ifdef	__AVR__
	uint8_t tmp, tval;
	uint32_t cval;

	/* lock interrupts */
	tmp = SREG;
	cli();
	cval = mt_counter;
	cval <<= 8;
	tval = TCNT2;

	/* if counter 2 signals an overflow, correct value now */
	if ((TIFR2 & _BV(TOV2)) != 0) {
		tval = TCNT2;
		cval += 256;
	}

	/* restore interrupt state */
	SREG = tmp;

	/* combine counter calues */
	cval |= tval;
	return cval;
#else	/* __AVR__ */
	return 0;
#endif	/* __AVR__ */
}

/* update MIDI timer value
 *
 * return		the new MIDI timer value
 */
static inline uint32_t mt_update_midi_timeval(void) {
	uint32_t cval, dval;

	/* get current counter and previous value */
	cval = mt_get_counter();
	dval = mt_midi_prevcnt;

	/* update previous value and calculate difference */
	mt_midi_prevcnt = cval;
	cval -= dval;
	cval &= 0xffffff;

	/* store and return new timer value */
	mt_midi_timeval = cval;
	return cval;
}

/* update machine timer value
 *
 * return		the new machine timer value
 */
static inline uint32_t mt_update_machine_timeval(void) {
	uint32_t cval, dval;

	/* get current counter and previous value */
	cval = mt_get_counter();
	dval = mt_machine_prevcnt;

	/* update previous value and calculate difference */
	mt_machine_prevcnt = cval;
	cval -= dval;
	cval &= 0xffffff;

	/* store and return new timer value */
	mt_machine_timeval = cval;
	return cval;
}

/* update sync timer
 *
 * timeval		new timer value for sync timer
 */
static inline void mt_update_sync_timer(uint32_t timeval) {
#ifdef	__AVR__
	uint8_t prescaler;
	uint32_t tv, diff;

	/* check current value against previous value */
	prescaler = 1;
	tv = mt_prev_timeval;
	if (tv > timeval) {
		diff = tv - timeval;
	} else {
		diff = timeval - tv;
	}
	mt_prev_timeval = timeval;

	/* allow jitter/change of 12.5% of previous value */
	tv >>= 3;
	if (diff > tv) {
		/* difference is too large, unlock sync timer */
		mt_synclock = 6;
		return;
	}

	/* calculate prescaler and timer value. this only works up to 64, as the
	 * steps switch from factor 8 to factor 4 after this, but this is okay
	 * here, as 0xffff * 64 == 4 millions, which means 0.5s with an 8MHz
	 * clock, which can't happen under normal conditions
	 */
	prescaler = 1;
	timeval -= 1;
	while (timeval >= 0x10000) {
		prescaler++;
		timeval >>= 3;
	}

	/* re-program timer 1. lost quite a lot of cycles up to here, but this
	 * isn't critical, it just produces a small jitter in the clock output.
	 */
	TCCR1B = (1 << WGM12) | (prescaler << CS10);
	OCR1A = timeval;

	/* reset timer and clear output compare flag to avoid triggering the next
	 * clock due to pending overflow interrupts immediately after the first
	 */
	TCNT1 = 0;
	TIFR1 = (1 << OCF1A);

	/* decrease lock counter if not yet locked */
	if (mt_synclock > 0)
		mt_synclock--;
#endif	/* __AVR__ */
}

/* get trigger bitmask for trigger number
 *
 * number		trigger number
 *
 * return		bit mask for trigger or 0, if invalid
 */
static inline uint8_t mt_get_trigger(uint8_t number) {
	switch (number) {
#ifdef	__AVR__
#ifdef MT_TRIG00
	case 0:
		return _BV(MT_TRIG00);
		break;
#endif	/* MT_TRIG00 */
#ifdef MT_TRIG01
	case 1:
		return _BV(MT_TRIG01);
		break;
#endif	/* MT_TRIG01 */
#ifdef MT_TRIG02
	case 2:
		return _BV(MT_TRIG02);
		break;
#endif	/* MT_TRIG02 */
#ifdef MT_TRIG03
	case 3:
		return _BV(MT_TRIG03);
		break;
#endif	/* MT_TRIG03 */
#ifdef MT_TRIG04
	case 4:
		return _BV(MT_TRIG04);
		break;
#endif	/* MT_TRIG04 */
#ifdef MT_TRIG05
	case 5:
		return _BV(MT_TRIG05);
		break;
#endif	/* MT_TRIG05 */
#ifdef MT_TRIG06
	case 6:
		return _BV(MT_TRIG06);
		break;
#endif	/* MT_TRIG06 */
#ifdef MT_TRIG07
	case 7:
		return _BV(MT_TRIG07);
		break;
#endif	/* MT_TRIG07 */
#ifdef MT_TRIG08
	case 8:
		return _BV(MT_TRIG08);
		break;
#endif	/* MT_TRIG08 */
#ifdef MT_TRIG09
	case 9:
		return _BV(MT_TRIG09);
		break;
#endif	/* MT_TRIG09 */
#ifdef MT_TRIG10
	case 10:
		return _BV(MT_TRIG10);
		break;
#endif	/* MT_TRIG10 */
#ifdef MT_TRIG11
		case 11:
		return _BV(MT_TRIG11);
		break;
#endif	/* MT_TRIG11 */
#ifdef MT_TRIG12
		case 12:
		return _BV(MT_TRIG12);
		break;
#endif	/* MT_TRIG12 */
#ifdef MT_TRIG13
		case 13:
		return _BV(MT_TRIG13);
		break;
#endif	/* MT_TRIG13 */
#ifdef MT_TRIG14
		case 14:
		return _BV(MT_TRIG14);
		break;
#endif	/* MT_TRIG14 */
#ifdef MT_TRIG15
		case 15:
		return _BV(MT_TRIG15);
		break;
#endif	/* MT_TRIG15 */
#endif	/* __AVR__ */
	default:
		return 0;
		break;
	}
}

/* serial interface driver functions (mt_serial.c) */
void mt_serial_init(void);
int16_t mt_serial_get(void);
uint8_t mt_serial_put(uint8_t data);

/* MIDI API functions (mt_midi.c) */
void mt_midi_init(void);
void mt_midi_receive(void);
void mt_midi_transmit(void);

/* TWI driver functions (mt_twi.c) */
void mt_twi_init(void);
void mt_twi_update(uint8_t value);

/* hardware API functions (mt_<driver>).c */
void mt_io_init(void);
uint16_t mt_button(void);
void mt_hwseq_start(void);
void mt_hwseq_stop(void);
void mt_hwseq_clock(void);
void mt_hwseq_trigger(void);
#ifndef	MT_HW_USESEQ
void mt_check_triggers(void);
#endif	/* MT_HW_USESEQ */

/* configuration API functions mt_config.c */
void mt_config_factory(void);
void mt_config_load(void);
void mt_config_save(void);
void mt_config_init(void);

/* configuration API functions mt_config_<impl>.c */
void mt_config_init_if(void);
void mt_config_handle(uint8_t abort);
uint8_t mt_config_getmidi(void);
void mt_config_midi(uint8_t status, uint8_t data);
uint8_t mt_config_allow_in(void);
uint8_t mt_config_allow_out(void);

/* encoder/display driver */
void mt_encdsp_init(void);
uint8_t mt_encdsp_getenc(void);
void mt_encdsp_cmd(uint8_t cmd);
void mt_encdsp_write(uint8_t data);

/* make eclipse online syntax check happy */
#ifndef	ISR
#ifdef __AVR__
#error ISR undefined
#endif	/* __AVR__ */
#define	ISR(a)	void a (void)
#endif

#endif /* MIDITRIGGS_H_ */
