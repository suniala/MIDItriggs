/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - Roland CR-78 hardware driver
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

#if defined(MT_HW_CR78)

#ifdef MT_HW_USESEQ
/* timstamp of last clock event generated by ISR */
volatile uint16_t mt_lastclock;

/* machine clock state */
volatile uint8_t mt_machclock;

/* startup delay counter */
volatile uint8_t mt_startdelay;

/* lost clocks counter */
volatile uint8_t mt_lostclocks;

/* write switch flag */
volatile uint8_t mt_writeswitch;

/* run-out counter for hardware sequencer */
volatile uint8_t mt_runout;

/* machine clock counter */
volatile uint8_t mt_machcount;
#endif	/* MT_HW_USESEQ */

#ifdef MT_HW_CFGBUTTON
/* button counter. 0 = button released, any other value = time button is pushed */
uint16_t mt_btcnt;
#endif	/* MT_HW_CFGBUTTON */

/* check external triggers */
#ifdef MT_HW_USESEQ
static inline void mt_check_triggers(void)
#else	/* MT_HW_USESEQ */
void mt_check_triggers(void)
#endif	/* MT_HW_USESEQ */
{
	uint8_t trig0, trig1, dir0, dir1, trig, dir, cnt, tmp;

	/* get current trigger signals */
	dir0 = MT_TRIGD0;
	dir1 = MT_TRIGD1;
	trig0 = MT_TRIGI0;
	trig1 = MT_TRIGI1;

	/* check all trigger signals */
	for (cnt = 0; cnt < MT_TRIGGERS; cnt++) {
		/* get hardware status of current trigger signal */
		tmp = 0;
		if (cnt < MT_TRIGSPLIT) {
			trig = trig0;
			dir = dir0;
		} else {
			trig = trig1;
			dir = dir1;
		}
		tmp = mt_get_trigger(cnt);
		if ((trig & tmp) != 0 && (dir & tmp) == 0)
			tmp = 1;
		else
			tmp = 0;

		/* get software status of current trigger signal */
		trig = mt_triggers[cnt];
		if ((trig & MT_TRIG_EXT) != 0) {
			/* if trigger is known as external trigger, reset if seen and no longer active */
			if ((trig & MT_TRIG_SEEN) != 0 && tmp == 0)
				mt_triggers[cnt] = 0;
		} else if (trig == 0) {
			/* if trigger is not active by software and hardware trigger is set, update trigger field */
			if (tmp != 0)
				mt_triggers[cnt] = MT_TRIG_EXT;
		}
	}
}

#ifdef	MT_HW_USESEQ
/* CR-78 specific MIDI clock update */
static inline void mt_cr78_clock(void) {
	uint16_t tmpc;
	uint8_t lcnt;

	/* if clock timer didn't just handle a clock, manually handle clock */
	tmpc = mt_timer;
	tmpc -= mt_lastclock;
	if (tmpc > 3) {
		mt_lastclock = mt_timer;
		mt_machclock ^= 1;
		if ((mt_machclock & 1) != 0) {
			mt_machcount++;
			if (mt_machcount >= 48) {
				mt_machcount = 0;
#ifdef	MT_HW_TWI
				/* update program number (RAM high address byte) */
				mt_twi_update(mt_midi_program);
#endif	/* MT_HW_TWI */
			}
			MT_CTLO |= _BV(MT_CLKOUT);

			/* turn off write switch */
			if (mt_writeswitch == 0)
				MT_CTLD &= ~_BV(MT_WRITESW);

			/* evil delay loop to wait at least 200�s for triggers to fire */
			lcnt = 200;
			while (--lcnt) {
				wdr();wdr();wdr();wdr();wdr();wdr();
			}

			/* scan triggers before passing a H->L clock transition */
			mt_check_triggers();
		} else {
			/* handle write switch */
			if (mt_writeswitch != 0) {
				MT_CTLD |= _BV(MT_WRITESW);
				wdr();wdr();wdr();
				mt_writeswitch = 0;
			}

			/* set clock to low */
			MT_CTLO &= ~_BV(MT_CLKOUT);
		}
	}
}
#endif	/* MT_HW_USESEQ */

/* initialize input/output ports */
void mt_io_init(void) {
#ifdef	MT_HW_USESEQ
	uint8_t tmp;
#endif	/* MT_HW_USESEQ */

	/* CR-78 has high-active triggers, so configure the ports as inputs
	 * without pullup resistor for "idle" mode, while configuring the pins
	 * to output and high level when a trigger gets fired.
	 */

	/* turn off pull-ups */
	MT_TRIGO0 &= ~MT_TRIGM0;
	MT_TRIGO1 &= ~MT_TRIGM1;

	/* all signals are inputs in the initial state */
	MT_TRIGD0 &= (uint8_t) ~MT_TRIGM0;
	MT_TRIGD1 &= (uint8_t) ~MT_TRIGM1;

#ifdef MT_HW_CFGBUTTON
	/* reset button counter */
	mt_btcnt = 0;

	/* configure button input */
	PORTB |= _BV(MT_BUTTON);
	DDRB &= ~(_BV(MT_BUTTON));
#endif	/* MT_HW_CFGBUTTON */

#ifdef	MT_HW_USESEQ
	/* turn off pull-ups */
	MT_CTLO &= ~(_BV(MT_STARTSW) | _BV(MT_RUNFF) | _BV(MT_CLKOUT) | _BV(
			MT_WRITESW));

	/* all signals are inputs in the initial state */
	MT_CTLD &= ~(_BV(MT_STARTSW) | _BV(MT_RUNFF) | _BV(MT_WRITESW));

	/* except clock output */
	MT_CTLD |= _BV(MT_CLKOUT);

	/* configure interrupt pins, pullup for clock input */
	PORTD &= ~_BV(MT_RUNNING);
	PORTD |= _BV(MT_CLOCK);
	DDRD &= ~(_BV(MT_RUNNING) | _BV(MT_CLOCK));

	/* enable external interrupts */
	EICRA = (1 << ISC10) | (1 << ISC00);
	EIMSK = (1 << INT1) | (1 << INT0);
	EIFR = (1 << INTF1) | (1 << INTF0);

	/* initialize variables */
	tmp = SREG;
	cli();
	mt_lastclock = mt_timer - 5;
	SREG = tmp;
	mt_machclock = 0;
	mt_lostclocks = 0;
	mt_startdelay = 0;
	mt_writeswitch = 0;
	mt_machcount = 0;
#endif	/* MT_HW_USESEQ */

#ifdef	MT_HW_TWI
	mt_twi_init();
	mt_twi_update(0);
#endif	/* MT_HW_TWI */
}

/* get button time, return 0 if button is not pushed
 *
 * return		The time the button is pushed in miliseconds
 */
#ifdef MT_HW_CFGBUTTON
uint16_t mt_button(void) {
	return mt_btcnt;
}
#endif	/* MT_HW_CFGBUTTON */

/* start hardware sequencer */
void mt_hwseq_start(void) {
#ifdef MT_HW_USESEQ
	uint8_t tmp;
	/* if machine is already running, we can't do anything */
	if ((MT_CTLI & _BV(MT_RUNFF)) == 0)
		return;

#ifdef	MT_HW_TWI
	/* update program number (RAM high address byte) */
	mt_twi_update(mt_midi_program);
#endif	/* MT_HW_TWI */

	/* initialize hardware clocking */
	mt_machclock = 1;
	mt_lostclocks = 0;
	mt_startdelay = 2;
	mt_synclock = 24;
	MT_CTLO |= _BV(MT_CLKOUT);
	tmp = SREG;
	cli();
	mt_lastclock = mt_timer - 5;
	SREG = tmp;
	mt_writeswitch = 0;
	mt_machcount = 0;
	MT_CTLD &= ~_BV(MT_WRITESW);

	/* loop here pushing the button and holding it until unit is started */
	while ((MT_CTLI & _BV(MT_RUNFF)) != 0) {
		/* release the start/stop switch */
		MT_CTLD &= ~_BV(MT_STARTSW);
		MT_CTLO &= ~_BV(MT_STARTSW);
		wdr();wdr();wdr();

		/* push and hold the start/stop switch */
		MT_CTLO |= _BV(MT_STARTSW);
		MT_CTLD |= _BV(MT_STARTSW);
		wdr();wdr();wdr();
	}
#endif	/* MT_HW_USESEQ */
}

/* stop hardware sequencer */
void mt_hwseq_stop(void) {
#ifdef MT_HW_USESEQ
	uint8_t tmp;

	/* loop here pushing and releasing the button until unit is stopped */
	while ((MT_CTLI & _BV(MT_RUNFF)) == 0) {
		/* push the start/stop switch */
		MT_CTLO |= _BV(MT_STARTSW);
		MT_CTLD |= _BV(MT_STARTSW);
		wdr();wdr();wdr();

		/* release the start/stop switch */
		MT_CTLD &= ~_BV(MT_STARTSW);
		MT_CTLO &= ~_BV(MT_STARTSW);
		wdr();wdr();wdr();
	}

	/* update sync system state (interrupts disabled) */
	tmp = SREG;
	cli();
	if (mt_syncmode >= MT_SYNC_MIDI_START) {
		if ((PIND & _BV(MT_RUNNING)) == 0)
			mt_syncmode = MT_SYNC_OFF;
		else {
			/* we potentially lost out clock and need free running,
			 * so force sync timer to be locked.
			 */
			mt_synclock = 0;
			mt_syncmode = MT_SYNC_MIDI_FREE;
		}
	}
	SREG = tmp;
	mt_writeswitch = 0;
	MT_CTLD &= ~_BV(MT_WRITESW);
#endif	/* MT_HW_USESEQ */
}

/* hardware sequencer clock handling */
void mt_hwseq_clock(void) {
#ifdef MT_HW_USESEQ
	uint8_t tmp;

	/* disable interrupts to avoid collisions */
	tmp = SREG;
	cli();
	if (mt_startdelay != 0 || (PIND & _BV(MT_RUNNING)) == 0)
		mt_lostclocks++;
	else
		mt_cr78_clock();
	SREG = tmp;
#endif	/* MT_HW_USESEQ */
}

/* got any trigger - set write switch */
void mt_hwseq_trigger(void) {
	if (mt_syncmode != MT_SYNC_OFF) {
		mt_writeswitch = 1;
		MT_CTLD |= _BV(MT_WRITESW);
	}
}

/* timer 0 compare match A ISR */
ISR(TIMER0_COMPA_vect) {
	uint8_t dir0, dir1, trig0, trig1;
	uint8_t cnt, tmp;

#ifdef MT_HW_USESEQ
	/* handle lost clocks */
	if (mt_lostclocks > 0) {
		if ((PIND & _BV(MT_RUNNING)) != 0) {
			tmp = mt_startdelay;
			if (tmp > 0) {
				tmp--;
				mt_startdelay = tmp;
			} else {
				mt_lostclocks--;
				tmp = SREG;
				cli();
				mt_cr78_clock();
				SREG = tmp;
				mt_startdelay = 1;
			}
		}
	}
#endif	/* MT_HW_USESEQ */

	/* handle trigger latching */
	tmp = mt_latchdelay;
	if (tmp > 0) {
		tmp--;
		mt_latchdelay = tmp;
		if (tmp == 0) {
			/* copy all new triggers to signal outputs in one transaction */
			for (cnt = 0; cnt < MT_TRIGGERS; cnt++) {
				tmp = mt_triggers[cnt];
				if ((tmp & MT_TRIG_EXT) == 0 && mt_triglatch[cnt] > tmp) {
					mt_triggers[cnt] = mt_triglatch[cnt];
					mt_triglatch[cnt] = 0;
				}
			}
		}
	}

	/* get port values and turn off triggers */
	dir0 = MT_TRIGD0;
	dir1 = MT_TRIGD1;
	trig0 = MT_TRIGO0;
	trig1 = MT_TRIGO1;
	dir0 &= (uint8_t) ~MT_TRIGM0;
	dir1 &= (uint8_t) ~MT_TRIGM1;
	trig0 &= (uint8_t) ~MT_TRIGM0;
	trig1 &= (uint8_t) ~MT_TRIGM1;

	/* enable triggers on active instruments */
	for (cnt = 0; cnt < MT_TRIGGERS + MT_VTRIGGERS; cnt++) {
		tmp = mt_triggers[cnt];
		if ((tmp & MT_TRIG_EXT) == 0 && tmp > 0) {
			mt_triggers[cnt] = tmp - 1;
			tmp = mt_get_trigger(cnt);
			if (cnt < MT_TRIGSPLIT) {
				dir0 |= tmp;
				trig0 |= tmp;
			} else {
				dir1 |= tmp;
				trig1 |= tmp;
			}
		}
	}

	/* write back port values */
	MT_TRIGO0 = trig0;
	MT_TRIGO1 = trig1;
	MT_TRIGD0 = dir0;
	MT_TRIGD1 = dir1;

#ifdef MT_HW_CFGBUTTON
	/* check button */
	if ((PINB & _BV(MT_BUTTON)) == 0) {
		uint16_t val;

		/* button is pressed, increment button time up to maximum */
		val = mt_btcnt + 1;
		if (val != 0)
			mt_btcnt = val;
	} else {
		/* button is released, set button time to 0 */
		mt_btcnt = 0;
	}
#endif	/* MT_HW_CFGBUTTON */

	/* increment generic timer */
	mt_timer++;
}

/* timer 1 output compare A ISR - MIDI clock out timer */
ISR(TIMER1_COMPA_vect) {
#ifdef MT_HW_USESEQ
	/* ignore if not yet locked */
	if (mt_synclock != 0)
		return;

	/* generate missing MIDI clocks while machine sequencer is running
	 * limit to 1 clock generated by this ISR (clock 1 is generated by
	 * the INT1 ISR) to avoid duplicate clocks on the next machine clock
	 */
	if (mt_syncmode == MT_SYNC_MACH_RUN) {
		if (mt_midi_clock < 2)
			mt_midi_clock++;
	}

	/* when under MIDI control, derive machine clock here */
	if (mt_syncmode == MT_SYNC_MIDI_RUN || mt_syncmode == MT_SYNC_MIDI_FREE) {
		mt_cr78_clock();
	}
#endif	/* MT_HW_USESEQ */
}

#ifdef MT_HW_USESEQ
/* INT0 ISR handling start/stop signal */
ISR(INT0_vect) {
	uint8_t run;

	/* get run state */
	run = PIND & _BV(MT_RUNNING);

	/* unlock sync timer */
	mt_synclock = 24;

	/* check if we are under MIDI control */
	if (mt_syncmode >= MT_SYNC_MIDI_START) {
		/* for a manual stop while under MIDI control, switch off sync */
		if (run == 0) {
			mt_syncmode = MT_SYNC_OFF;
			mt_runout = 12;
		}
	} else {
		/* update state on manual control */
		if (run == 0)
			mt_syncmode = MT_SYNC_MACH_STOP;
		else {
			mt_syncmode = MT_SYNC_MACH_START;
			mt_machcount = 0;
		}
		mt_midi_clock = 0xff;
	}
}

/* INT1 ISR handling external clock (high edge) */
ISR(INT1_vect) {
	uint8_t cnt, clkstate;

	/* get state of external clock signal */
	clkstate = PIND & _BV(MT_CLOCK);

	/* get machine sync timer value on high edges */
	if (clkstate != 0) {
		uint32_t counter;
		counter = mt_update_machine_timeval();
		/* if not under MIDI control, update sync timer */
		if (mt_syncmode < MT_SYNC_MIDI_START) {
			cnt = counter & 1;
			counter >>= 1;
			counter += cnt;
			mt_update_sync_timer(counter);

			/* if machine sequencer is running, generate MIDI clocks */
			if (mt_syncmode == MT_SYNC_MACH_RUN) {
				/* this always sets clock to 1, as all other clocks are interpolated by the sync timer */
				mt_midi_clock = 1;
			}
		}
	} else {
		/* check if clock timer is locked */
		if (mt_synclock != 0)
			mt_midi_clock = 2;

		/* check if we are under MIDI control */
		if (mt_syncmode < MT_SYNC_MIDI_START) {
			/* scan triggers before passing a H->L clock transition */
			mt_check_triggers();
		}
	}

	/* check if machine sequencer has control */
	if (mt_syncmode < MT_SYNC_MIDI_START) {
		/* give 12 more clock transitions after stop */
		if (mt_syncmode == MT_SYNC_MACH_START || mt_syncmode
				== MT_SYNC_MACH_RUN)
			mt_runout = 12;

		/* during normal run or while running out, give clocks */
		if (mt_runout > 0) {
			mt_runout--;
			/* pass through clock signal to machine */
			if (clkstate == 0) {
				/* handle write switch */
				if (mt_writeswitch != 0) {
					MT_CTLD |= _BV(MT_WRITESW);
					wdr();wdr();wdr();
					mt_writeswitch = 0;
				}
				MT_CTLO &= ~_BV(MT_CLKOUT);
			} else {
				mt_machcount++;
				if (mt_machcount >= 48) {
					mt_machcount = 0;
#ifdef	MT_HW_TWI
					/* update program number (RAM high address byte) */
					mt_twi_update(mt_midi_program);
#endif	/* MT_HW_TWI */
				}
				MT_CTLO |= _BV(MT_CLKOUT);

				/* turn off write switch */
				if (mt_writeswitch == 0)
					MT_CTLD &= ~_BV(MT_WRITESW);
			}
		} else {
			/* otherwise, lock clock to high */
			MT_CTLO |= _BV(MT_CLKOUT);
			mt_machclock = 1;
#ifdef	MT_HW_TWI
			/* update program number (RAM high address byte) */
			mt_twi_update(mt_midi_program);
#endif	/* MT_HW_TWI */
		}
	}
}

#endif	/* MT_HW_USESEQ */

#endif	/* MT_HW_CR78 */
