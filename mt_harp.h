/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - Hohner Automatic Rhythm Player hardware driver definitions
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

#ifndef MT_HARP_H_
#define MT_HARP_H_

/* Hardware interfacing signals
 *
 * Trigger signals:
 * 10 Trigger outputs, TTL-level, low-active, external pullup, triggers on h->l clock transition
 *
 * Machine sequencer signals:
 * 1 Counter highest bit as bar number. 0 when idle, 1 for 1st bar, 0 for 2nd bar.
 * 1 Clock signal, random state when idle, 24 pulses per bar (6 ppqn), low-active.
 * 1 Start-switch, 0 when stopped, mirror of clock with some pulses missing, when running
 *
 * Configuration button:
 * 1 Configure button, low-active, internal pullup
 *
 * Instrument mapping for HARP (first prototype board):
 * 00	Cymbal
 * 01	Maracas
 * 02	Hi Hat
 * 03	Bass Drum
 * 04	Snare Drum
 * 05	High Bongo
 * 06	Low Conga
 * 07	Claves
 * 08	Low Bongo
 * 09	Cow Bell
 */

#if defined(MT_HW_HARP)

/* default instrument map */
#define	MT_INSTMAP		{ \
	{ 39, 255, 100, 1, { 'C', 'y', 'm', 'b', 'a', 'l', ' ', ' ' } },\
	{ 44, 255, 100, 1, { 'M', 'a', 'r', 'a', 'c', 'a', 's', ' ' } },\
	{ 37, 255, 100, 1, { 'H', 'i', ' ', 'H', 'a', 't', ' ', ' ' } },\
	{ 36, 255, 100, 1, { 'B', 'a', 's', 's', 'D', 'r', 'u', 'm' } },\
	{ 38, 255, 100, 1, { 'S', 'n', 'a', 'r', 'e', ' ', ' ', ' ' } },\
	{ 45, 255, 100, 1, { 'H', '.', ' ', 'B', 'o', 'n', 'g', 'o' } },\
	{ 41, 255, 100, 1, { 'L', '.', ' ', 'C', 'o', 'n', 'g', 'a' } },\
	{ 42, 255, 100, 1, { 'C', 'l', 'a', 'v', 'e', 's', ' ', ' ' } },\
	{ 43, 255, 100, 1, { 'L', '.', ' ', 'B', 'o', 'n', 'g', 'o' } },\
	{ 40, 255, 100, 1, { 'C', 'o', 'w', ' ', 'B', 'e', 'l', 'l' } },\
};

/* feedback instruments */
#define	MT_FBACK0		3		/* Bass drum */
#define	MT_FBACK1		4		/* Snare drum */

/* number of triggers */
#define	MT_TRIGGERS		10

/* trigger split point */
#define	MT_TRIGSPLIT	5

/* trigger port pins on trigger port 0 */
#define	MT_TRIG00		PB1
#define	MT_TRIG01		PB2
#define	MT_TRIG02		PB3
#define	MT_TRIG03		PB4
#define	MT_TRIG04		PB5

/* trigger port 0 */
#define	MT_TRIGO0		PORTB
#define	MT_TRIGD0		DDRB
#define	MT_TRIGI0		PINB
#define	MT_TRIGM0		(_BV(MT_TRIG00) | _BV(MT_TRIG01) | _BV(MT_TRIG02) | _BV(MT_TRIG03) | _BV(MT_TRIG04))

/* trigger port pins on trigger port 1 */
#define	MT_TRIG05		PC1
#define	MT_TRIG06		PC2
#define	MT_TRIG07		PC3
#define	MT_TRIG08		PC4
#define	MT_TRIG09		PC5

/* trigger port 1 */
#define	MT_TRIGO1		PORTC
#define	MT_TRIGD1		DDRC
#define	MT_TRIGI1		PINC
#define	MT_TRIGM1		(_BV(MT_TRIG05) | _BV(MT_TRIG06) | _BV(MT_TRIG07) | _BV(MT_TRIG08) | _BV(MT_TRIG09))

/* control port pins */
#define	MT_BARCNT		PD2
#define	MT_CLOCK		PD3
#define	MT_START		PD4
#define	MT_BUTTON		PD5

/* control port */
#define	MT_CTLO			PORTD
#define	MT_CTLD			DDRD
#define	MT_CTLI			PIND

/* trigger length in miliseconds */
#define	MT_TRIGLEN		8

#endif	/* MT_HW_HARP */

#if defined(MT_HW_HARP2)

/* default instrument map */
#define	MT_INSTMAP		{ \
	{ 39, 255, 100, 1, { 'C', 'y', 'm', 'b', 'a', 'l', ' ', ' ' } },\
	{ 44, 255, 100, 1, { 'M', 'a', 'r', 'a', 'c', 'a', 's', ' ' } },\
	{ 37, 255, 100, 1, { 'H', 'i', ' ', 'H', 'a', 't', ' ', ' ' } },\
	{ 36, 255, 100, 1, { 'B', 'a', 's', 's', 'D', 'r', 'u', 'm' } },\
	{ 38, 255, 100, 1, { 'S', 'n', 'a', 'r', 'e', ' ', ' ', ' ' } },\
	{ 45, 255, 100, 1, { 'H', '.', ' ', 'B', 'o', 'n', 'g', 'o' } },\
	{ 41, 255, 100, 1, { 'L', '.', ' ', 'C', 'o', 'n', 'g', 'a' } },\
	{ 42, 255, 100, 1, { 'C', 'l', 'a', 'v', 'e', 's', ' ', ' ' } },\
	{ 43, 255, 100, 1, { 'L', '.', ' ', 'B', 'o', 'n', 'g', 'o' } },\
	{ 40, 255, 100, 1, { 'C', 'o', 'w', ' ', 'B', 'e', 'l', 'l' } },\
};

/* feedback instruments */
#define	MT_FBACK0		3		/* Bass drum */
#define	MT_FBACK1		4		/* Snare drum */

/* number of triggers */
#define	MT_TRIGGERS		10

/* trigger split point */
#define	MT_TRIGSPLIT	4

/* trigger port pins on trigger port 0 */
#define	MT_TRIG00		PD4
#define	MT_TRIG01		PD5
#define	MT_TRIG02		PD6
#define	MT_TRIG03		PD7

/* trigger port 0 */
#define	MT_TRIGO0		PORTD
#define	MT_TRIGD0		DDRD
#define	MT_TRIGI0		PIND
#define	MT_TRIGM0		(_BV(MT_TRIG00) | _BV(MT_TRIG01) | _BV(MT_TRIG02) | _BV(MT_TRIG03))

/* trigger port pins on trigger port 1 */
#define	MT_TRIG04		PB0
#define	MT_TRIG05		PB1
#define	MT_TRIG06		PB2
#define	MT_TRIG07		PB3
#define	MT_TRIG08		PB4
#define	MT_TRIG09		PB5
#define	MT_TRIG10		PB6
#define	MT_TRIG11		PB7

/* trigger port 1 */
#define	MT_TRIGO1		PORTB
#define	MT_TRIGD1		DDRB
#define	MT_TRIGI1		PINB
#define	MT_TRIGM1		(_BV(MT_TRIG04) | _BV(MT_TRIG05) | _BV(MT_TRIG06) | _BV(MT_TRIG07) | _BV(MT_TRIG08) | _BV(MT_TRIG09) | _BV(MT_TRIG10) | _BV(MT_TRIG11))

#ifdef MT_HW_CFGENCDSP
/* encoder/display port configuration */
#define	MT_DSP_B0		PC0
#define	MT_DSP_B1		PC1
#define	MT_DSP_B2		PC2
#define	MT_DSP_B3		PC3
#define	MT_DSP_DATAMASK	(_BV(MT_DSP_B0) | _BV(MT_DSP_B1) | _BV(MT_DSP_B2) | _BV(MT_DSP_B3))
#define	MT_DSP_RS		PC4		/* command(0)/data(1) */
#define	MT_DSP_RW		PC5		/* read(1)/write(0) */
#define	MT_DSP_E		PD2		/* display selection, high-active */
#define	MT_DSP_ENC		PD3		/* encoder selection, low-active */
#define	MT_DSP_DDO		PORTC
#define	MT_DSP_DDD		DDRC
#define	MT_DSP_DDI		PINC
#define	MT_DSP_DCO		PORTD
#define	MT_DSP_DCD		DDRD
#define	MT_DSP_DCI		PIND
#else	/* MT_HW_CFGENCDSP */
/* control port */
#define	MT_CTLO			PORTD
#define	MT_CTLD			DDRD
#define	MT_CTLI			PIND

/* control port pins */
#define	MT_BARCNT		PD2
#define	MT_CLOCK		PD3
#define	MT_START		PD4
#define	MT_BUTTON		PD5
#endif	/* MT_HW_CFGENCDSP */

/* trigger length in miliseconds */
#define	MT_TRIGLEN		8

#endif	/* MT_HW_HARP2 */

#endif /* MT_HARP_H_ */
