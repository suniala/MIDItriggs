/* vim: set ts=4 sw=4: */
/*
 * MIDItriggs - configuration program main handler
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

/* configuration data structure */
struct mt_config_data mt_config_data;

/* default instrument map */
struct mt_map_inst_out PROGMEM mt_config_instmap[] = MT_INSTMAP

/* update 8-bit CRC
 *
 * crc			The current CRC value
 * value		The data value for the update
 *
 * return		The new CRC value
 */
uint8_t crc8_update(uint8_t crc, uint8_t value) {
	uint8_t cnt;
	uint8_t poly = CRC8_POLY;

	cnt = 8;
	do {

#ifdef __AVR__
		uint8_t tmp;

		__asm__ volatile(
				"mov	%2, %0\n\t"
				"lsl	%0\n\t"
				"eor	%2, %1\n\t"
				"lsl	%1\n\t"
				"sbrc	%2, 7\n\t"
				"eor	%0, %3\n\t"
				: "=r" (crc), "=r" (value), "=&r" (tmp), "=r" (poly)
				: "0" (crc), "1" (value), "3" (poly)
		);
#else
		crc = (crc << 1) ^ ((crc ^ value) & 0x80 ? poly : 0);
		value <<= 1;
#endif
	} while (--cnt);

	return crc;
}

/* initialize configuration to factory defaults */
void mt_config_factory(void) {
	uint16_t i;
	uint8_t val, *p, *d;
	struct mt_map_inst_out *map;

	/* clear config data to 0xff */
	p = (uint8_t *) &mt_config_data;
	for (i = 0; i < sizeof(mt_config_data); i++) {
		*(p++) = 0xff;
	}

	/* set MIDI configuration */
	mt_config_data.midi_in_channel = 0;
	mt_config_data.midi_out_channel = 0;
	mt_config_data.midi_in_omni = 1;
	mt_config_data.midi_in_sync = 1;
	mt_config_data.midi_out_use_in = 1;
	mt_config_data.midi_out_enable = 1;
	mt_config_data.midi_out_sync = 1;

	/* set display configuration */
	mt_config_data.display_line1 = 0;
	mt_config_data.display_line2 = 0;

	/* reset input mappings */
	for(i = 0; i < 128; i++) {
		wdr();
		mt_config_data.inst_in[i].trigger = 255;
		mt_config_data.inst_in[i].triglen = MT_TRIGLEN;
	}

	/* set default mappings and instrument names */
	for (i = 0; i < sizeof(mt_config_instmap) / sizeof(struct mt_map_inst_out); i++) {
		wdr();
		map = &(mt_config_instmap[i]);
		val = pgm_read_byte(&(map->note));
		mt_config_data.inst_in[val].trigger = i;
		mt_config_data.inst_in[val].triglen = MT_TRIGLEN;
		mt_config_data.inst_out[i].note = val;
		mt_config_data.inst_out[i].channel = pgm_read_byte(&(map->channel));
		mt_config_data.inst_out[i].velocity = pgm_read_byte(&(map->velocity));
		mt_config_data.inst_out[i].length = pgm_read_byte(&(map->length));
		p = map->name;
		d = mt_config_data.inst_out[i].name;
		for (val = 0; val < 8; val++)
			*(d++) = pgm_read_byte(p++);
	}

	/* reset remaining instruments */
	for (; i < MT_MAXTRIG; i++) {
		wdr();
		mt_config_data.inst_out[i].note = 255;
		mt_config_data.inst_out[i].channel = 255;
		mt_config_data.inst_out[i].velocity = 100;
		mt_config_data.inst_out[i].length = 1;
		p = mt_config_data.inst_out[i].name;
		for (val = 0; val < 8; val++)
			*(p++) = ' ';
	}

	/* set display text to blanks */
	wdr();
	p = mt_config_data.display_text[0];
	for (i = 0; i < sizeof(mt_config_data.display_text); i++)
		*(p++) = ' ';

	/* set default BPM for internal clock generator */
	mt_config_data.out_bpm = 120;
}

/* load config from EEPROM and check CRC */
void mt_config_load(void) {
	uint16_t i;
	uint8_t crc, val, *p;

	/* read configuration from EEPROM */
	p = (uint8_t *) &mt_config_data;
	crc = CRC8_INIT;
	for (i = 0; i < sizeof(mt_config_data); i++) {
		val = eeprom_read_byte((uint8_t *) (i + EEP_RESVD));
		crc = crc8_update(crc, val);
		*(p++) = val;
		wdr();
	}

	if (crc != 0)
		mt_config_factory();
}

/* save configuration to EEPROM */
void mt_config_save(void) {
	uint16_t i;
	uint8_t *p, crc, val;

	/* write all differing bytes to EEPROM, update CRC */
	p = (uint8_t *) &mt_config_data;
	crc = CRC8_INIT;
	for (i = 0; i < sizeof(mt_config_data) - 1; i++) {
		val = *(p++);
		crc = crc8_update(crc, val);
		if (val != eeprom_read_byte((uint8_t *) (i + EEP_RESVD)))
			eeprom_write_byte((uint8_t *) (i + EEP_RESVD), val);wdr();
	}

	/* write CRC byte to config buffer and EEPROM, if necessary */
	*(p++) = crc;
	if (crc != eeprom_read_byte((uint8_t *) (i + EEP_RESVD)))
		eeprom_write_byte((uint8_t *) (i + EEP_RESVD), crc);
}

/* initialize configuration system */
void mt_config_init(void) {
	/* get initial config from EEPROM, load factory defaults on failure */
	mt_config_load();

	/* initialize interface specific stuff */
	mt_config_init_if();
}
