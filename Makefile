# MIDItriggs make file
# Copyright (C) 2009-2011 Michael Kukat <michael_AT_mik-music.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# MCU type
MCU	= atmega168

# clock frequency
CLKF	= 8000000

# object files for resulting binary
OBJECTS	= fwcheck.o main.o mt_config.o mt_config_button.o mt_cr78.o mt_harp.o mt_midi.o mt_serial.o mt_twi.o

# default target
all:	miditriggs.hex

# MIDItriggs main program
%.o:	%.c
	avr-gcc -Wall -Werror -pedantic -O2 -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields \
	-std=gnu99 -mmcu=$(MCU) -DF_CPU=$(CLKF) -c -o $@ $<

%.o:	%.S
	avr-gcc -Wall -Werror -pedantic -O2 -mmcu=$(MCU) -DF_CPU=$(CLKF) -c -o $@ $<

miditriggs.elf:	$(OBJECTS)
	avr-gcc -mmcu=$(MCU) -o $@ $(OBJECTS)

miditriggs.hex:	miditriggs.elf
	avr-objcopy -O ihex $< $@

# create SysEx file for firmware update via MIDIboot
miditriggs.syx:	miditriggs.hex
	../MIDIboot/hex2syx.pl <$< >$@

# clean up all the stuff we created
clean:
	rm -f $(OBJECTS) miditriggs.elf miditriggs.hex miditriggs.syx
