/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - Roland CR-78 hardware driver definitions
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

#ifndef MT_CR78_H_
#define MT_CR78_H_

/* Hardware interfacing signals
 *
 * Trigger signals, MIDItriggs CN101:
 *
 * 11 Trigger outputs, high-active, pulse width depends on machine clock
 * trigger driver base resistor 56KOhms, giving 60µA drive current on active
 * trigger. This resistor is replaced by 1 15KOhms resistor and 1 39KOhms
 * resistor (15KOhms on latch side), while MIDItriggs is connected between.
 * This gives 104-110µA drive current when MIDItriggs drives the triggers and
 * 74µA drive current when MIDItriggs is switched to input and CR-78 drives
 * the triggers. MIDItriggs so always can override the CR-78 triggers, the
 * worst case is CR-78 providing trigger and MIDItriggs switched to low, which
 * results in about 0.25V base voltage, which should not be enough to fire the
 * trigger. Anyway, this situation should be avoided by the user.
 * The trigger signals are connected to MIDItriggs CN101, pins 3-13,
 * connecting them to PD4-7 and PB0-6
 *
 * Triggers are controlled by the CR-78 MCU in the following way:
 * On L->H transition of the machine clock, the MCU calculates all the trigger
 * signals and writes them to the latches, while there is a delay of about
 * 200ms between the clock transition and the triggers coming available.
 * On the H->L transition of the machine clock, the latches are reset by this
 * transition.
 *
 * The reset input of the latches is asynchronous, so all the trigger signals
 * from the CR-78 can be held off by keeping the machine clock signal low.
 *
 * The configuration button is connected to CN101 pin 14, which is PB7.
 *
 * Control signals, MIDItriggs CN102:
 * 1	memory board pin 1					GND
 * 2	memory board pin 2					VCC
 * 3	start stop switch		D101		PC0			OUT/open
 * 4	start stop ff			R120		PC1			IN
 * 5	machine clock			thru-hole	PC2			OUT/driven
 * 6	write switch			IC120-13	PC3			OUT/open
 * 7	memory board pin 4					PC4/SDA		PERI
 * 8	memory board pin 5					PC5/SCL		PERI
 * 9	start stop post-fade	IC122-9		PD2/INT0	IN/interrupt
 * 10	clock generator			IC120-2		PD3/INT1	IN+pullup/interrupt
 *
 * Clock signals:
 * Clock needs to be intercepted, a trace cut is necessary directly after
 * IC 120, pin 2 (GL-9B). The output of this IC is the normal machine clock
 * and goes to pin 10 of MIDItriggs CN102, connecting it to PD3/INT1. On the
 * CR-78 logic board, R206 has to be removed, it is replaced by the pullup
 * on PD3 being enabled.
 * The clock output from MIDItriggs comes from PC2 via pin 5 of CN102 and
 * goes to the other side of the cut trace.
 *
 * Start/stop signals:
 * The start/stop signal to the CPU is connected to MIDItriggs CN102, pin 9,
 * connecting it to PD2/INT0. This signal is behind the fade in/out circuitry
 * and provides the machine run signal used for the CR-78 sequencer.
 * To check the real state of the start/stop toggle, another signal is needed,
 * which is acquired from IC109, pin 2 and connected to CN102, pin 4, which is
 * PC1. The start/stop toggle signal (CN102, pin 3, which is PC0) is connected
 * to IC109, pin 3. This high-active signal is fed to this pin via 2 diodes,
 * MIDItriggs doesn't need another diode, as this can be controlled with the
 * data direction register. This signal is used as output to control the
 * start/stop of the hardware sequencer.
 *
 * Write switch signal:
 * Goes to IC120, pin 13 and is controlled by MIDItriggs CN102, pin 6, which
 * is PC3. This signal is low-active, open drain.
 *
 * Peripheral interface:
 * Additional peripherals like the memory bank select are connected to the TWI
 * on CN102, pins 7 and 8 and powered by GND/VCC on pins 1 and 2 if necessary.
 *
 * Instrument mapping for CR-78:
 * 00	Accent
 * 01	BD		Bass Drum
 * 02	SD		Snare Drum
 * 03	CY		Cymbal
 * 04	M		Maracas
 * 05	HB		High Bongo
 * 06	LC		Low Conga
 * 07	LB		Low Bongo
 * 08	C		Claves
 * 09	HH		Hi Hat
 * 10	RS		Rim Shot
 */

#if defined(MT_HW_CR78)

/* default instrument map */
#define	MT_INSTMAP		{ \
	{ 46, 255, 100, 1, { 'A', 'c', 'c', 'e', 'n', 't', ' ', ' ' } },\
	{ 36, 255, 100, 1, { 'B', 'a', 's', 's', 'D', 'r', 'u', 'm' } },\
	{ 38, 255, 100, 1, { 'S', 'n', 'a', 'r', 'e', ' ', ' ', ' ' } },\
	{ 39, 255, 100, 1, { 'C', 'y', 'm', 'b', 'a', 'l', ' ', ' ' } },\
	{ 44, 255, 100, 1, { 'M', 'a', 'r', 'a', 'c', 'a', 's', ' ' } },\
	{ 45, 255, 100, 1, { 'H', '.', ' ', 'B', 'o', 'n', 'g', 'o' } },\
	{ 41, 255, 100, 1, { 'L', '.', ' ', 'C', 'o', 'n', 'g', 'a' } },\
	{ 43, 255, 100, 1, { 'L', '.', ' ', 'B', 'o', 'n', 'g', 'o' } },\
	{ 42, 255, 100, 1, { 'C', 'l', 'a', 'v', 'e', 's', ' ', ' ' } },\
	{ 37, 255, 100, 1, { 'H', 'i', ' ', 'H', 'a', 't', ' ', ' ' } },\
	{ 40, 255, 100, 1, { 'R', 'i', 'm', ' ', 'S', 'h', 'o', 't' } },\
};

/* feedback instruments */
#define	MT_FBACK0		1		/* Bass drum */
#define	MT_FBACK1		2		/* Snare drum */

/* number of triggers */
#define	MT_TRIGGERS		11

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

/* trigger port 1 */
#define	MT_TRIGO1		PORTB
#define	MT_TRIGD1		DDRB
#define	MT_TRIGI1		PINB
#define	MT_TRIGM1		(_BV(MT_TRIG04) | _BV(MT_TRIG05) | _BV(MT_TRIG06) | _BV(MT_TRIG07) | _BV(MT_TRIG08) | _BV(MT_TRIG09) | _BV(MT_TRIG10))

/* control port pins */
#define	MT_BUTTON		PB7
#define	MT_RUNNING		PD2
#define	MT_CLOCK		PD3
#define	MT_STARTSW		PC0
#define	MT_RUNFF		PC1
#define	MT_CLKOUT		PC2
#define	MT_WRITESW		PC3

/* control port */
#define	MT_CTLO			PORTC
#define	MT_CTLD			DDRC
#define	MT_CTLI			PINC

/* trigger length in miliseconds */
#define	MT_TRIGLEN		8

#endif	/* MT_HW_CR78 */

#endif /* MT_CR78_H_ */
