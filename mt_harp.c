/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - Hohner Automatic Rhythm Player hardware driver
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

#if defined(MT_HW_HARP) || defined(MT_HW_HARP2)

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
	uint8_t trig0, trig1, trig, cnt, tmp;

	/* get current trigger signals */
	trig0 = MT_TRIGI0;
	trig1 = MT_TRIGI1;

	/* check all trigger signals */
	for (cnt = 0; cnt < MT_TRIGGERS; cnt++) {
		/* get hardware status of current trigger signal */
		tmp = 0;
		if (cnt < MT_TRIGSPLIT) {
			trig = trig0;
		} else {
			trig = trig1;
		}
		tmp = mt_get_trigger(cnt);
		if ((trig & tmp) == 0)
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

/* initialize input/output ports */
void mt_io_init(void) {
	/* Use ports as some sort of "open drain" configuration by disabling
	 * the pull-up resistors by writing 0s to the port output values and
	 * controlling the direction bits instead of the output bits for the
	 * trigger signals. The ports can also be used as inputs, while the
	 * trigger output (thus direction signal) is off (logic 1), which is
	 * used for the MIDI out feature and for the other control signals.
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
	MT_CTLO |= _BV(MT_BUTTON);
	MT_CTLD &= ~(_BV(MT_BUTTON));
#endif	/* MT_HW_CFGBUTTON */

#ifdef	MT_HW_USESEQ
	/* turn off pull-ups */
	MT_CTLO &= ~(_BV(MT_BARCNT) | _BV(MT_CLOCK) | _BV(MT_START));

	/* all signals are inputs in the initial state */
	MT_CTLD &= ~(_BV(MT_BARCNT) | _BV(MT_CLOCK) | _BV(MT_START));

	/* enable external interrupts */
	EICRA = (2 << ISC10) | (3 << ISC00);
	EIMSK = (1 << INT1) | (1 << INT0);
	EIFR = (1 << INTF1) | (1 << INTF0);
#endif	/* MT_HW_USESEQ */

#ifdef	MT_HW_TWI
	mt_twi_init();
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
}

/* stop hardware sequencer */
void mt_hwseq_stop(void) {
}

/* hardware sequencer clock handling */
void mt_hwseq_clock(void) {
}

/* unassigned trigger handling */
void mt_hwseq_trigger(void) {
}

/* timer 0 compare match A ISR */
ISR(TIMER0_COMPA_vect) {
	uint8_t trig0, trig1;
	uint8_t cnt, tmp;

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
					mt_triggers[cnt] = mt_triglatch[cnt] & 0x7f;
					mt_triglatch[cnt] = 0;
				}
			}
		}
	}

	/* get port values and turn off triggers */
	trig0 = MT_TRIGD0;
	trig1 = MT_TRIGD1;
	trig0 &= (uint8_t) ~MT_TRIGM0;
	trig1 &= (uint8_t) ~MT_TRIGM1;

	/* enable triggers on active instruments */
	for (cnt = 0; cnt < MT_TRIGGERS + MT_VTRIGGERS; cnt++) {
		tmp = mt_triggers[cnt];
		if ((tmp & MT_TRIG_EXT) == 0 && tmp > 0) {
			mt_triggers[cnt] = tmp - 1;
			tmp = mt_get_trigger(cnt);
			if (cnt < MT_TRIGSPLIT)
				trig0 |= tmp;
			else
				trig1 |= tmp;
		}
	}

	/* write back port values */
	MT_TRIGD0 = trig0;
	MT_TRIGD1 = trig1;

#ifdef MT_HW_CFGBUTTON
	/* check button */
	if ((MT_CTLI & _BV(MT_BUTTON)) == 0) {
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
	 * limit to 3 clocks generated by this ISR (clock 1 is generated by
	 * the INT1 ISR) to avoid duplicate clocks on the next machine clock
	 */
	if (mt_syncmode == MT_SYNC_MACH_RUN) {
		if (mt_midi_clock < 4)
			mt_midi_clock++;
	}
#endif	/* MT_HW_USESEQ */
}

#ifdef MT_HW_USESEQ
/* those 2 interrupts run into some sort of race condition, because the
 * pattern start signal occurs in parallel with the clock signal, you
 * never know which ISR is triggered first, this makes things a bit
 * difficult if you want to handle this right. But as the pattern counter
 * is currently unused, it doesn't matter.
 */

/* INT0 ISR handling pattern start signal (high edge) */
ISR(INT0_vect) {
}

/* INT1 ISR handling external clock (low edge) */
ISR(INT1_vect) {
	uint32_t counter;
	uint8_t tmp;

	/* update machine sync timer value */
	counter = mt_update_machine_timeval();

	/* if not under MIDI control, update sync timer */
	if (mt_syncmode < MT_SYNC_MIDI_START) {
		/* factor 4 for MIDI clock versus machine clock */
		counter >>= 1;
		tmp = counter & 1;
		counter >>= 1;
		counter += tmp;
		mt_update_sync_timer(counter);

		/* if machine sequencer is running, generate MIDI clocks */
		if (mt_syncmode == MT_SYNC_MACH_RUN) {
			/* this always sets clock to 1, as all other clocks are interpolated by the sync timer */
			mt_midi_clock = 1;
		}
	}

	/* check external trigger signals */
	mt_check_triggers();
}
#endif	/* MT_HW_USESEQ */

#endif	/* MT_HW_HARP / MT_HW_HARP2 */
