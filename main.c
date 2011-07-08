/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - A simple MIDI trigger interface
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

/* sync system state */
volatile mt_syncmode_t mt_syncmode;

/* generic timer */
volatile uint16_t mt_timer;

/* bits 8-23 of 24 bit counter to determine machine clock speed */
volatile uint16_t mt_counter;

/* 24 bit timer value for MIDI clock */
volatile uint32_t mt_midi_prevcnt;
volatile uint32_t mt_midi_timeval;

/* 24 bit timer value for machine clock */
volatile uint32_t mt_machine_prevcnt;
volatile uint32_t mt_machine_timeval;

/* 24 bit previous timer value to check for jitter */
volatile uint32_t mt_prev_timeval;

/* sync timer lock counter, timer is valid, when this is 0 */
volatile uint8_t mt_synclock;

/* main program */
int main(int argc, char **argv) {
	/* save some power by disabling unused peripherals */
	PRR = (0 << PRTWI) | (0 << PRTIM2) | (0 << PRTIM0) | (0 << PRTIM1) | (1
			<< PRSPI) | (0 << PRUSART0) | (1 << PRADC);

	/* initialize MIDI interface */
	mt_midi_init();

	/* initialize hardware interface */
	mt_io_init();

	/* reset counter values and states */
	mt_syncmode = MT_SYNC_OFF;
	mt_timer = 0;
	mt_counter = 0;
	mt_midi_prevcnt = 0;
	mt_midi_timeval = 0;
	mt_machine_prevcnt = 0;
	mt_machine_timeval = 0;
	mt_prev_timeval = 0;
	mt_synclock = 200;

	/* Timer 0 is a generic global timer to handle timeouts, set trigger
	 * signals and more. It is programmed to give a clock rate of 1KHz, firing
	 * the ISR in 1ms intervals. WGM 2 - CTC, prescaler 64, output compare
	 * match A interrupt, normal port functions.
	 */
	TCCR0A = (0 << COM0A0) | (0 << COM0B0) | (2 << WGM00);
	TCCR0B = (0 << WGM02) | (3 << CS00);
	OCR0A = F_CPU / 64 / 1000 - 1;
	TIMSK0 = (1 << OCIE0A);
	TIFR0 = (1 << OCF0A);

	/* Timer 1 is the synchronization PLL-like timer which is triggered 24
	 * times per quarter note. When in MIDI-in mode, it is synchronized to
	 * incoming MIDI sync data, being able to compensate for a missing
	 * clock or to provide further clocks after stop has been received.
	 * In MIDI-out mode, is is synchronized to the machine clock that usually
	 * is slower than the MIDI clock, so it can compensate for the missing
	 * machine clock cycles.
	 * It is initialized with a timer value for about 120 BPM.
	 * 24 ppqn * 120 BPM = 2880 ppm = 48 pps
	 */
	TCCR1A = (0 << COM1A0) | (0 << COM1B0) | (0 << WGM10);
	TCCR1B = (1 << WGM12) | (4 << CS10);
	OCR1A = F_CPU / 256 / 48 - 1;
	TCNT1 = 0;
	TIFR1 = (1 << OCF1A);
	TIMSK1 = (1 << OCIE1A);

	/* Counter 2 is the 8 bit part of a 24 bit counter using a software
	 * counter for the upper 16 bits. This counter is used to get the clock
	 * cycles between events to program timer 1 for 24 ppqn synchronization.
	 */
	ASSR = 0;
	GTCCR = 0;
	TCCR2A = (0 << COM2A0) | (0 << COM2B0) | (0 << WGM20);
	TCCR2B = (0 << WGM22) | (1 << CS20);
	TCNT2 = 0;
	TIFR2 = (1 << TOV2);
	TIMSK2 = (1 << TOIE2);

	/* enable interrupts */
	sei();

	/* initialize configuration */
	mt_config_init();

	/* main loop */
	while (1) {
		/* handle MIDI transmit */
		mt_midi_transmit();

		/* handle MIDI receive */
		mt_midi_receive();

		/* handle config subsystem */
		mt_config_handle(0);

		/* reset watchdog timer */
		asm volatile("wdr");
	}

	/* never reached */
	return 0;
}

/* timer 2 overflow ISR - update counter MSB */
ISR(TIMER2_OVF_vect) {
	mt_counter++;
}
