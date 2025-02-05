MCU = atmega328p

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
FORMAT = ihex

CFLAGS = -O2 -g2 -std=gnu99 -Wall -mmcu=$(MCU) -D F_CPU=16000000UL

SRC = main.c
TARGET = display

OBJ = $(SRC:.c=.o) $(ASRC:.S=.o) 

AVRDUDE = sudo avrdude
# Debug wire does not allow a chip erase so use -D to disable it
AVRDUDE_FLAGS = -p m328p -c atmelice_dw -D -v
#AVRDUDE_FLAGS = -p m328p -c atmelice_isp -v

all: $(TARGET).hex

clean:
	@echo 
	rm -f $(TARGET).hex
	rm -f $(TARGET).elf
	rm -f $(TARGET).eep
	rm -f $(TARGET).sym
	rm -f $(OBJ)

# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $@
	$(OBJCOPY) -O $(FORMAT) $< $@
# $(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

#%.eep: %.elf
#	@echo
#	@echo $(MSG_EEPROM) $@
#	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
#	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Link: create ELF output file from object files.
%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) $(CFLAGS) $(OBJ) --output $@ $(LDFLAGS)

# Compile: create object files from C source files.
%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(CFLAGS) $< -o $@

program: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(TARGET).hex:i
#$(AVRDUDE) $(AVRDUDE_FLAGS) -DV -U flash:w:$(TARGET).hex:i

writeFUSEdebugWire:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U hfuse:w:0x99:m

writeFUSEisp:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U hfuse:w:0xD9:m

.PHONY: all clean program 
