/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - configuration program for single button config
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

#ifdef MT_HW_CFGBUTTON

/* configuration mode */
enum {
	MT_CFGM_IDLE = 0,
	MT_CFGM_MIDI_CHANNEL,
	MT_CFGM_INST_ASSIGN_IN,
	MT_CFGM_CLOCK_SYNC,
	MT_CFGM_MIDI_OUT,
	MT_CFGM_INST_ASSIGN_OUT,
	MT_CFGM_CLOCK_OUT,
	MT_CFGM_DISCARD,
	MT_CFGM_SAVE,
	MT_CFGM_END
} mt_config_mode;

/* button state */
uint8_t mt_config_buttonstate;

/* sequencer data */
uint8_t mt_config_seq[8];

/* sequencer step, 0xff if stopped */
uint8_t mt_config_seq_step;

/* sequencer timer */
uint16_t mt_config_seq_timer;

/* interface specific initialization */
void mt_config_init_if(void) {
	/* enable pull-up and input for the button */
	MT_CTLO |= _BV(MT_BUTTON);
	MT_CTLD &= ~(_BV(MT_BUTTON));

	/* stop sequencer */
	mt_config_seq_step = 0xff;

	/* initialize button state */
	mt_config_buttonstate = 0;

	/* initialize config state engine */
	mt_config_mode = MT_CFGM_IDLE;
}

/* stop sequencer */
void mt_config_seq_stop(void) {
	mt_config_seq_step = 0xff;
}

/* start sequencer
 *
 * pos			The position to start at
 */
void mt_config_seq_start(uint8_t pos) {
	uint8_t trigger;

	/* check validity of position */
	if (pos > 8)
		return;

	/* initialize sequencer timer and start sequencer */
	trigger = SREG;
	cli();
	mt_config_seq_timer = mt_timer;
	SREG = trigger;
	mt_config_seq_step = pos;

	/* send trigger for this step if available */
	trigger = mt_config_seq[pos];
	if (trigger < MT_TRIGGERS) {
		mt_triggers[trigger] = MT_TRIGLEN;
	}
}

/* clear sequencer data */
void mt_config_seq_clear(void) {
	uint8_t i;
	for (i = 0; i < 8; i++)
		mt_config_seq[i] = 0xff;
}

/* set sequencer slot
 *
 * slot			The slot to set data at
 * data			The data to set
 */
void mt_config_seq_set(uint8_t slot, uint8_t data) {
	if (slot >= 8)
		return;
	mt_config_seq[slot] = data;
}

/* update sequencer if necessary */
void mt_config_seq_run(void) {
	uint16_t timer, etime;
	uint8_t step, tmp;

	/* get current step */
	step = mt_config_seq_step;
	if (step >= 8)
		return;

	/* check timer if we need to run the next sequencer step */
	tmp = SREG;
	cli();
	timer = mt_timer;
	SREG = tmp;
	etime = mt_config_seq_timer;
	etime = timer - etime;
	if (etime >= MT_SEQTIME) {
		mt_config_seq_timer = timer;
		step++;
		if (step >= 8)
			step = 0;
		mt_config_seq_step = step;

		/* send trigger for this step if available */
		step = mt_config_seq[step];
		if (step < MT_TRIGGERS) {
			mt_triggers[step] = MT_TRIGLEN;
		}
	}
}

/* exit configuration system
 *
 * save			1 if data is to be saved in EEPROM
 */
void mt_config_exit(uint8_t save) {
#ifdef	MT_HW_CFGSEQRUN
	mt_hwseq_stop();
#endif	/* MT_HW_CFGSEQRUN */
	/* save configuration to EEPROM if desired */
	if (save != 0) {
		mt_config_save();
	}
	mt_config_seq_stop();
	mt_config_mode = MT_CFGM_IDLE;
}

/* show current config item
 *
 * value		Data value of current item of 0xff if no value
 * start		Start position of sequencer
 */
void mt_config_show(uint8_t value, uint8_t start) {
	uint8_t tmp;

	/* stop and clear sequencer data */
	mt_config_seq_stop();
	mt_config_seq_clear();

	/* write current config mode in binary notation to first 3 seq slots */
	tmp = mt_config_mode - 1;
	mt_config_seq_set(0, ((tmp & 4) == 0) ? MT_FBACK0 : MT_FBACK1);
	mt_config_seq_set(1, ((tmp & 2) == 0) ? MT_FBACK0 : MT_FBACK1);
	mt_config_seq_set(2, ((tmp & 1) == 0) ? MT_FBACK0 : MT_FBACK1);

	/* write value to 5th sequencer slot */
	mt_config_seq_set(4, value);

	/* start sequencer at given position */
	mt_config_seq_start(start);
}

/* handle configuration button
 *
 * type			0 if short press, 1 if long press
 */
void mt_config_button(uint8_t type) {
	uint8_t i;

	if (type == 0) { /* short button push */
		if (mt_config_mode == MT_CFGM_IDLE)
			return;

		/* handle short button in configuration mode */
		mt_config_mode++;
		if (mt_config_mode == MT_CFGM_END) {
			mt_config_mode = MT_CFGM_IDLE + 1;
		}
		switch (mt_config_mode) {
		case MT_CFGM_MIDI_CHANNEL: /* set MIDI omni mode */
			if (mt_config_data.midi_in_omni != 0)
				mt_config_show(MT_FBACK0, 0);
			else
				mt_config_show(MT_FBACK1, 0);
			break;
		case MT_CFGM_INST_ASSIGN_IN: /* delete all assignments */
			mt_config_show(MT_FBACK1, 0);
			break;
		case MT_CFGM_CLOCK_SYNC: /* toggle clock synchronization */
			mt_config_show(mt_config_data.midi_in_sync == 0 ? MT_FBACK0
					: MT_FBACK1, 0);
			break;
		case MT_CFGM_MIDI_OUT: /* toggle MIDI out */
			mt_config_show(mt_config_data.midi_out_enable == 0 ? MT_FBACK0
					: MT_FBACK1, 0);
			break;
		case MT_CFGM_INST_ASSIGN_OUT: /* delete all assignments */
			mt_config_show(MT_FBACK1, 0);
			break;
		case MT_CFGM_CLOCK_OUT: /* toggle clock output */
			mt_config_show(mt_config_data.midi_out_sync == 0 ? MT_FBACK0
					: MT_FBACK1, 0);
			break;
		default:
			mt_config_show(0xff, 0);
			break;
		}
	} else { /* long button push */
		switch (mt_config_mode) {
		case MT_CFGM_IDLE: /* enter config system */
			mt_config_mode++;
#ifdef	MT_HW_CFGSEQRUN
			mt_syncmode = MT_SYNC_BLOCKED;
			mt_hwseq_start();
#endif	/* MT_HW_CFGSEQRUN */
			if (mt_config_data.midi_in_omni != 0)
				mt_config_show(MT_FBACK0, 0);
			else
				mt_config_show(MT_FBACK1, 0);
			break;
		case MT_CFGM_MIDI_CHANNEL: /* set MIDI omni mode */
			mt_config_data.midi_in_omni = 1;
			mt_config_show(MT_FBACK0, 4);
			break;
		case MT_CFGM_INST_ASSIGN_IN: /* delete all assignments */
			for (i = 0; i < 128; i++) {
				mt_config_data.inst_in[i].trigger = 0xff;
				mt_config_data.inst_in[i].triglen = MT_TRIGLEN;
			}
			mt_config_show(MT_FBACK0, 4);
			break;
		case MT_CFGM_CLOCK_SYNC: /* toggle clock synchronization */
			mt_config_data.midi_in_sync ^= 1;
			mt_config_show(mt_config_data.midi_in_sync == 0 ? MT_FBACK0
					: MT_FBACK1, 4);
			break;
		case MT_CFGM_MIDI_OUT: /* toggle MIDI out */
			mt_config_data.midi_out_enable ^= 1;
			mt_config_data.midi_out_use_in = 1;
			mt_config_show(mt_config_data.midi_out_enable == 0 ? MT_FBACK0
					: MT_FBACK1, 4);
			break;
		case MT_CFGM_INST_ASSIGN_OUT: /* delete all assignments */
			for (i = 0; i < MT_TRIGGERS; i++) {
				mt_config_data.inst_out[i].note = 0xff;
				mt_config_data.inst_out[i].channel = 0xff;
				mt_config_data.inst_out[i].velocity = 100;
				mt_config_data.inst_out[i].length = 1;
			}
			mt_config_show(MT_FBACK0, 4);
			break;
		case MT_CFGM_CLOCK_OUT: /* toggle clock output */
			mt_config_data.midi_out_sync ^= 1;
			mt_config_show(mt_config_data.midi_out_sync == 0 ? MT_FBACK0
					: MT_FBACK1, 4);
			break;
		case MT_CFGM_DISCARD: /* discard configuration */
			mt_config_show(MT_FBACK0, 4);
			mt_config_exit(0);
			break;
		case MT_CFGM_SAVE: /* save configuration */
			mt_config_show(MT_FBACK1, 4);
			mt_config_exit(1);
			break;
		default:
			break;
		}
	}
}

/* handle MIDI message (note-on only)
 *
 * status		The status byte of the MIDI message
 * data			The first data byte
 */
void mt_config_midi(uint8_t status, uint8_t data) {
	uint8_t tmp;

	/* only handle note messages yet */
	if((status & 0xf0) != 0x90) return;

	switch (mt_config_mode) {
	case MT_CFGM_MIDI_CHANNEL: /* learn MIDI channel with each incoming message */
		mt_config_data.midi_in_channel = status & 0x0f;
		mt_config_data.midi_in_omni = 0;
		mt_config_show(MT_FBACK1, 4);
		break;
	case MT_CFGM_INST_ASSIGN_IN: /* assign incoming notes to instruments */
		tmp = mt_config_data.inst_in[data].trigger + 1;
		if (tmp >= MT_TRIGGERS)
			tmp = 0xff;
		mt_config_data.inst_in[data].trigger = tmp;
		mt_config_data.inst_in[data].triglen = MT_TRIGLEN;
		mt_config_show(tmp, 4);
		break;
	case MT_CFGM_CLOCK_SYNC: /* enable/disable clock synchronization */
	case MT_CFGM_MIDI_OUT: /* enable/disable MIDI out */
		break;
	case MT_CFGM_INST_ASSIGN_OUT: /* assign instruments to outgoing notes */
		/* assign incoming note as output for instrument configured as incoming instrument for this note */
		tmp = mt_config_data.inst_in[data].trigger;
		if (tmp < MT_TRIGGERS) {
			mt_config_data.inst_out[tmp].note = data;
			mt_config_data.inst_out[tmp].channel = 0xff;
			mt_config_data.inst_out[tmp].velocity = 100;
			mt_config_data.inst_out[tmp].length = 1;
		}
		mt_config_show(tmp, 4);
		break;
	case MT_CFGM_CLOCK_OUT: /* enable/disable clock out */
	default:
		break;
	}
}

/* report if config system wants MIDI messages
 *
 * return		1 if messages shall be routed to config system, 0 if not
 */
uint8_t mt_config_getmidi(void) {
	if (mt_config_mode == MT_CFGM_IDLE)
		return 0;
	return 1;
}

/* configuration system main handler */
void mt_config_handle(uint8_t abort) {
	uint16_t bval;

	/* check for abort flag */
	if (abort != 0) {
		if (mt_config_mode != MT_CFGM_IDLE) {
			mt_config_seq_stop();
			mt_config_mode = MT_CFGM_IDLE;
		}
		return;
	}

	/* check button */
	bval = mt_button();
	if (mt_config_buttonstate == 0) {
		/* use 10ms debounce delay */
		if (bval > 10) {
			mt_config_buttonstate = 1;
		}
	} else {
		if (bval == 0) {
			/* button released, check if short or long push */
			if (mt_config_buttonstate == 1) {
				/* button activity for short push */
				mt_config_button(0);
			}
			mt_config_buttonstate = 0;
		} else if (mt_config_buttonstate == 1 && bval >= MT_BTIME) {
			/* button activity for long push */
			mt_config_button(1);
			mt_config_buttonstate = 2;
		}
	}

	/* handle sequencer */
	mt_config_seq_run();
}

/* allow MIDI in? */
uint8_t mt_config_allow_in(void) {
	if (mt_config_mode != MT_CFGM_IDLE)
		return 0;
	return 1;
}

/* allow MIDI out? */
uint8_t mt_config_allow_out(void) {
	if (mt_config_mode != MT_CFGM_IDLE)
		return 0;
	return 1;
}

#endif	/* MT_HW_CFGBUTTON */

