# Makefile to build librfm69
#
# Simplified version from: https://github.com/hexagon5un/AVR-Programming

# https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
# avr-gcc -mmcu=atmega328p -print-multi-directory
MCU = avr5
# avr-gcc -mmcu=avr64ea28 -print-multi-directory
# MCU = avrxmega2

MAIN = librfm69.c
SRC =

CC = avr-gcc
AR = avr-ar

CFLAGS = -mmcu=$(MCU)
CFLAGS += -O2 -I.
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
CFLAGS += -Wall -Wstrict-prototypes
CFLAGS += -g -ggdb
CFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections -mrelax
CFLAGS += -std=gnu99
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105523
# CFLAGS += --param=min-pagesize=0
CFLAGS += -c

ARFLAGS = rcs

MAKEFLAGS += -r

TARGET = $(strip $(basename $(MAIN)))
SRC += $(TARGET).c

OBJ = $(SRC:.c=.o) 
OBJ = $(SRC:.S=.o)

$(TARGET).o: librfm69.h utils.h Makefile

all: $(TARGET).a
	
%.a: %.o
	$(AR) $(ARFLAGS) $(TARGET).a $(TARGET).o

%.o: $(SRC)
	$(CC) $(CFLAGS) $(SRC) --output $@ 

clean:
	rm -f $(TARGET).a $(TARGET).hex $(TARGET).obj \
	$(TARGET).o $(TARGET).d $(TARGET).eep $(TARGET).lst \
	$(TARGET).lss $(TARGET).sym $(TARGET).map $(TARGET)~ \
	$(TARGET).eeprom
