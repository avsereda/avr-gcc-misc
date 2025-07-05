SHELL		:= /bin/sh
CC 		:= avr-gcc
CPU_CLOCK	:= 16000000UL
MCU_DEVICE	:= atmega2560
CFLAGS 		:= -std=gnu99 -Wall -Wsign-compare -Wsign-conversion -I. -Os \
			-DF_CPU=$(CPU_CLOCK) -DJSMN_PARENT_LINKS -DJSMN_STRICT \
			-D__ASSERT_USE_STDERR -mmcu=$(MCU_DEVICE)

LIBS 		:= -Wl,-u,vfprintf -lprintf_flt
OBJCOPY		:= avr-objcopy
OBJCOPY_FLAGS	:=
AVRDUDE		:= avrdude

ifndef ($(port))
	port := /dev/ttyUSB0
endif

AVRDUDE_CONF	:= avrdude.conf
AVRDUDE_FLAGS	:= -C $(AVRDUDE_CONF) -c arduino -p $(MCU_DEVICE) -P $(port) -b 115200 -cwiring

hex		:= firmware.hex
eeprom		:= firmware.eeprom
elf 		:= firmware.elf
src		:= $(wildcard *.c)
objects 	:= $(patsubst %.c,%.o,$(filter %.c,$(src)))

.SILENT:

.PHONY: clean

$(elf): $(objects)
	$(CC) -mmcu=$(MCU_DEVICE)  $(LIBS) $(objects) -o $(elf)

$(hex): $(elf)
	$(OBJCOPY) $(OBJCOPY_FLAGS) -O ihex -R .eeprom $(elf) $(hex)

$(eeprom): $(elf)
	$(OBJCOPY) $(OBJCOPY_FLAGS) -O ihex -j .eeprom			\
		--set-section-flags=.eeprom=alloc,load			\
	  	--no-change-warnings					\
	  	--change-section-lma .eeprom=0				\
		$(elf) $(eeprom)

clean:
	rm -f $(elf) $(hex) $(eeprom) *.o

flash: $(hex) $(eeprom)
	$(AVRDUDE) $(AVRDUDE_FLAGS) -D -U flash:w:$(hex):i

test:
	./run-unittests.sh

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $< -o $@


