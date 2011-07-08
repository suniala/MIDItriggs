/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - MIDI function layer
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

/* trigger counters/flags */
volatile uint8_t mt_triggers[MT_TRIGGERS + MT_VTRIGGERS];

/* latch input for triggers if latched triggers are in used */
volatile uint8_t mt_triglatch[MT_TRIGGERS];

/* delay counter for synced triggers */
volatile uint8_t mt_latchdelay;

/* trigger summary for display output */
uint16_t mt_trigsum;

/* store sent notes for triggers to send correct note-off events */
uint8_t mt_trigsent[MT_TRIGGERS];

#ifdef MT_HW_USESEQ
/* counter for MIDI clock out. if this value changes, send a MIDI clock */
volatile uint8_t mt_midi_clock;

/* counter for clocks already sent */
uint8_t mt_midi_clock_sent;
#endif	/* MT_HW_USESEQ */

#ifdef	MT_HW_PGMCHG
/* current program number */
uint8_t mt_midi_program;
#endif	/* MT_HW_PGMCHG */

/* MIDI data storage */
uint8_t mt_channel;
uint8_t mt_midi_rxstatus;
uint8_t mt_midi_remain;
uint8_t mt_midi_store;
uint8_t mt_midi_txstatus;
uint16_t mt_midi_holdstatus;

/* main MIDI function init */
void mt_midi_init(void) {
	uint8_t cnt;

	/* initialize serial interface */
	mt_serial_init();

	/* initialize MIDI stack */
	mt_midi_rxstatus = 0;
	mt_midi_txstatus = 0;
	mt_midi_remain = 0;
	cnt = SREG;
	cli();
	mt_midi_holdstatus = mt_timer;
	SREG = cnt;
#ifdef	MT_HW_PGMCHG
	mt_midi_program = 0;
#endif	/* MT_HW_PGMCHG */

#ifdef MT_HW_USESEQ
	/* initialize sequencer state */
	mt_midi_clock = 0xff;
	mt_midi_clock_sent = 0xff;
#endif	/* MT_HW_USESEQ */

	/* reset triggers */
	for (cnt = 0; cnt < MT_TRIGGERS + MT_VTRIGGERS; cnt++)
		mt_triggers[cnt] = 0;

	for (cnt = 0; cnt < MT_TRIGGERS; cnt++) {
		mt_triglatch[cnt] = 0;
		mt_trigsent[cnt] = 0xff;
	}
	mt_trigsum = 0;

	/* reset latch delay counter */
	mt_latchdelay = 0;
}

/* MIDI receive handler */
void mt_midi_receive(void) {
	int16_t in;

	/* check for MIDI message and handle it if available */
	while ((in = mt_serial_get()) >= 0) {
		/* check if we care about MIDI sync */
		if (mt_config_data.midi_in_sync != 0 && in >= 0xf8
				&& mt_config_allow_in() != 0) {
			if (in == 0xf8) {
				uint32_t synctimer;

				/* update sync timer value */
				synctimer = mt_update_midi_timeval();

				/* handle MIDI clock for hardware sequencer */
				if (mt_syncmode == MT_SYNC_MIDI_RUN) {
					/* while running, update sync timer */
					mt_update_sync_timer(synctimer);
					mt_hwseq_clock();
				} else if (mt_syncmode == MT_SYNC_MIDI_START) {
					/* when just started, don't update sync timer */
					mt_syncmode = MT_SYNC_MIDI_RUN;
					mt_hwseq_clock();
				}
			} else if ((in == 0xfa || in == 0xfb) && mt_syncmode == MT_SYNC_OFF) {
				/* handle MIDI start and continue the same way */
				mt_syncmode = MT_SYNC_MIDI_START;
				mt_hwseq_start();
			} else if (in == 0xfc && (mt_syncmode == MT_SYNC_MIDI_RUN
					|| mt_syncmode == MT_SYNC_MIDI_START)) {
				/* handle MIDI stop */
				mt_syncmode = MT_SYNC_MIDI_STOP;
				mt_hwseq_stop();
			}
		}

		/* handle MIDI byte */
		if (in >= 0x80) {
			/* ignore system realtime messages */
			if (in < 0xf8) {
				/* ignore system common messages, but reset status */
				if (in < 0xf0) {
					/* handle channel messages */
					mt_midi_rxstatus = in;
					in &= 0xf0;
					/* handle note on */
					if (in == 0x90)
						mt_midi_remain = 2;
#ifdef	MT_HW_PGMCHG
					else if (in == 0xc0)
						mt_midi_remain = 1;
#endif	/* MT_HW_PGMCHG */
					else
						mt_midi_remain = 0;
				} else
					mt_midi_rxstatus = 0;
			}
		} else {
			/* handle data byte */
			switch (mt_midi_remain) {
			case 1:
				if ((mt_midi_rxstatus & 0xf0) == 0x90) {
					uint8_t trg, len;

					mt_midi_remain = 2;
					/* handle MIDI note on message if omni or channel match or config wants MIDI */
					if (in != 0 && ((mt_midi_rxstatus & 0x0f)
							== mt_config_data.midi_in_channel
							|| mt_config_data.midi_in_omni != 0
							|| mt_config_getmidi() != 0)) {
#if MT_VTRIGGERS > 0
						/* first virtual trigger is the MIDI activity trigger */
						mt_triggers[MT_TRIGGERS] = 10;
#endif	/* MT_VTRIGGERS > 0 */
						if (mt_config_getmidi() == 0) {
							if (mt_config_allow_in() != 0) {
								/* if config doesn't want MIDI, handle trigger */
								trg
										= mt_config_data.inst_in[mt_midi_store].trigger;
								len
										= mt_config_data.inst_in[mt_midi_store].triglen;
								len &= 0x7f;
								if (trg < MT_TRIGGERS) {
									mt_triglatch[trg] = len;
									mt_trigsum |= (1 << trg);
									/* hard-coded trigger latch delay of 2ms */
									mt_latchdelay = 2;
								} else
									/* inform sequencer about "unused trigger" event */
									mt_hwseq_trigger();
							}
						} else {
							/* send message to config system */
							mt_config_midi(mt_midi_rxstatus, mt_midi_store);
						}
					}
#ifdef	MT_HW_PGMCHG
				} else if ((mt_midi_rxstatus & 0xf0) == 0xc0) {
					/* handle MIDI program change message if omni or channel match or config wants MIDI */
					if ((mt_midi_rxstatus & 0x0f)
							== mt_config_data.midi_in_channel
							|| mt_config_data.midi_in_omni != 0
							|| mt_config_getmidi() != 0) {
						if (mt_config_getmidi() == 0) {
							if (mt_config_allow_in() != 0) {
								/* if config doesn't want MIDI, handle program change */
								mt_midi_program = in;
							}
						} else {
							/* send message to config system */
							mt_config_midi(mt_midi_rxstatus, in);
						}
					}
#endif	/* MT_HW_PGMCHG */
				}
				break;
			case 2:
				mt_midi_store = in;
				mt_midi_remain--;
				break;
			default:
				break;
			}
		}
	}
}

/* handle MIDI transmission of sync messages and triggers */
void mt_midi_transmit(void) {
	uint8_t cnt, status;
	uint16_t trigsum, timer;

	/* get current timer value */
	status = SREG;
	cli();
	timer = mt_timer;
	SREG = status;

	/* handle sync engine state transitions */
	status = 0;
#ifdef	MT_HW_USESEQ
	if (mt_syncmode == MT_SYNC_MACH_START) {
		mt_syncmode = MT_SYNC_MACH_RUN;
		/* send start */
		status = 0xfa;
	} else if (mt_syncmode == MT_SYNC_MACH_STOP) {
		mt_syncmode = MT_SYNC_OFF;
		/* send stop */
		status = 0xfc;
	} else if (mt_syncmode == MT_SYNC_MACH_RUN) {
		/* clock output */
		cnt = mt_midi_clock;
		if (cnt != mt_midi_clock_sent && cnt != 0xff) {
			mt_midi_clock_sent = cnt;
			status = 0xf8;
		}
	}
#endif	/* MT_HW_USESEQ */

	/* check if MIDI out is enabled and available */
	if (mt_config_allow_out() != 0 && mt_config_data.midi_out_enable != 0) {
		uint8_t master, note, velocity;

		/* check if sync out is enabled */
		if (status != 0 && mt_config_data.midi_out_sync != 0) {
			/* handle MIDI sync out*/
			mt_serial_put(status);
		}
#ifndef	MT_HW_USESEQ
		/* check for external trigger signals */
		mt_check_triggers();
#endif	/* MT_HW_USESEQ */

		/* get master out channel */
		master = mt_config_data.midi_out_channel;
		if (mt_config_data.midi_out_use_in != 0) {
			if (mt_config_data.midi_in_omni != 0)
				master = 0;
			else
				master = mt_config_data.midi_in_channel;
		}

		/* check all trigger states */
		if ((timer - mt_midi_holdstatus) > 300)
			mt_midi_txstatus = 0xff;
		trigsum = 0;
		for (cnt = 0; cnt < MT_TRIGGERS; cnt++) {
			if ((mt_triggers[cnt] & (MT_TRIG_EXT | MT_TRIG_SEEN))
					== MT_TRIG_EXT) {
				trigsum |= (1 << MT_TRIGGERS);
				/* if unseen external trigger is found, determine parameters for send */
				mt_triggers[cnt] |= MT_TRIG_SEEN;
				note = mt_config_data.inst_out[cnt].note;
				if (note < 128) {
#if MT_VTRIGGERS > 0
					/* first virtual trigger is the MIDI activity trigger */
					mt_triggers[MT_TRIGGERS] = 10;
#endif	/* MT_VTRIGGERS > 0 */
					status = mt_config_data.inst_out[cnt].channel;
					if (status > 15)
						status = master;
					status |= 0x90;
					velocity = mt_config_data.inst_out[cnt].velocity;
					mt_trigsent[cnt] = note;

					/* transmit note-on for trigger */
					if (mt_midi_txstatus != status) {
						mt_midi_txstatus = status;
						mt_serial_put(status);
					}
					mt_serial_put(note);
					mt_serial_put(velocity);
					mt_midi_holdstatus = timer;
				}
			}
			trigsum >>= 1;
		}
		mt_trigsum |= trigsum;
		for (cnt = 0; cnt < MT_TRIGGERS; cnt++) {
			if (mt_trigsent[cnt] < 128) {
				note = mt_trigsent[cnt];
				mt_trigsent[cnt] = 255;

				/* transmit note-off for trigger */
				status = mt_config_data.inst_out[cnt].channel;
				if (status > 15)
					status = master;
				status |= 0x90;
				if (mt_midi_txstatus != status) {
					mt_midi_txstatus = status;
					mt_serial_put(status);
				}
				mt_serial_put(note);
				mt_serial_put(0);
				mt_midi_holdstatus = timer;
			}
		}
	}
}
