
#
#  1. Modify the line containing "ARDUINO_INSTALL_DIR" to point to the directory that
#     contains the Arduino installation (for example, under macOS, this
#     might be /Applications/Arduino.app/Contents/Resources/Java).
#
#  2. Modify the line containing "UPLOAD_PORT" to refer to the filename
#     representing the USB or serial connection to your Arduino board
#     (e.g. UPLOAD_PORT = /dev/tty.USB0).  If the exact name of this file
#     changes, you can use * as a wild card (e.g. UPLOAD_PORT = /dev/tty.usb*).
#
#  3. Set the line containing "MCU" to match your board's processor. Set
#     "PROG_MCU" as the AVR part name corresponding to "MCU". You can use the
#     following command to get a list of correspondences: `avrdude -c alf -p x`
#     Older boards are atmega8 based, newer ones like Arduino Mini, Bluetooth
#     or Diecimila have the atmega168.  If you're using a LilyPad Arduino,
#     change F_CPU to 8000000. If you are using Gen7 electronics, you
#     probably need to use 20000000. Either way, you must regenerate
#     the speed lookup table with create_speed_lookuptable.py.
#
#  4. Type "make" and press enter to compile/verify your program.
#
#  5. Type "make upload", reset your Arduino board, and press enter to
#     upload your program to the Arduino board.
#
# Note that all settings at the top of this file can be overridden from
# the command line with, for example, "make HARDWARE_MOTHERBOARD=71"
#
# To compile for RAMPS (atmega2560) with Arduino 1.6.9 at root/arduino you would use...
#
#   make ARDUINO_VERSION=10609 AVR_TOOLS_PATH=/root/arduino/hardware/tools/avr/bin/ \
#   HARDWARE_MOTHERBOARD=1200 ARDUINO_INSTALL_DIR=/root/arduino
#
# To compile and upload simply add "upload" to the end of the line...
#
#   make ARDUINO_VERSION=10609 AVR_TOOLS_PATH=/root/arduino/hardware/tools/avr/bin/ \
#   HARDWARE_MOTHERBOARD=1200 ARDUINO_INSTALL_DIR=/root/arduino upload
#
# If uploading doesn't work try adding the parameter "AVRDUDE_PROGRAMMER=wiring" or
# start upload manually (using stk500) like so:
#
#   avrdude -C /root/arduino/hardware/tools/avr/etc/avrdude.conf -v -p m2560 -c stk500 \
#   -U flash:w:applet/Marlin.hex:i -P /dev/ttyUSB0
#
# Or, try disconnecting USB to power down and then reconnecting before running avrdude.
#

# This defines the board to compile for (see boards.h for your board's ID)
# 1020 in mega2560 RAMPS 1.4
HARDWARE_MOTHERBOARD ?= 1020

ifeq ($(OS),Windows_NT)
  # Windows
  ARDUINO_INSTALL_DIR ?= ${HOME}/Arduino
  ARDUINO_USER_DIR ?= ${HOME}/Arduino
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    # Linux
    ARDUINO_INSTALL_DIR ?= /opt/arduino-1.8.19
    ARDUINO_USER_DIR ?= ${HOME}/Arduino
  endif
  ifeq ($(UNAME_S),Darwin)
    # Darwin (macOS)
    ARDUINO_INSTALL_DIR ?= /Applications/Arduino.app/Contents/Java
    ARDUINO_USER_DIR ?= ${HOME}/Documents/Arduino
    AVR_TOOLS_PATH ?= /Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/
  endif
endif

# Arduino source install directory, and version number
# On most linuxes this will be /usr/share/arduino
ARDUINO_INSTALL_DIR  ?=
ARDUINO_VERSION      ?= 10819

# The installed Libraries are in the User folder
ARDUINO_USER_DIR ?= ${HOME}/Arduino

# You can optionally set a path to the avr-gcc tools.
# Requires a trailing slash. For example, /usr/local/avr-gcc/bin/
AVR_TOOLS_PATH ?= $(ARDUINO_INSTALL_DIR)/hardware/tools/avr/bin/

# Programmer configuration
UPLOAD_RATE        ?= 57600
AVRDUDE_PROGRAMMER ?= arduino
# On most linuxes this will be /dev/ttyACM0 or /dev/ttyACM1
UPLOAD_PORT        ?= /dev/ttyUSB0

# Directory used to build files in, contains all the build files, from object
# files to the final hex file on linux it is best to put an absolute path
# like /home/username/tmp .
BUILD_DIR          ?= applet

# This defines whether Liquid_TWI2 support will be built
LIQUID_TWI2        ?= 0

# This defines if Wire is needed
WIRE               ?= 1

# This defines if Tone is needed (i.e., SPEAKER is defined in Configuration.h)
# Disabling this (and SPEAKER) saves approximately 350 bytes of memory.
TONE               ?= 0

# This defines if U8GLIB is needed (may require RELOC_WORKAROUND)
U8GLIB             ?= 0

# This defines whether to include the Trinamic TMCStepper library
TMC                ?= 0

# This defines whether to include the AdaFruit NeoPixel library
NEOPIXEL           ?= 0

############
# Try to automatically determine whether RELOC_WORKAROUND is needed based
# on GCC versions:
#   https://www.avrfreaks.net/comment/1789106#comment-1789106

CC_MAJ:=$(shell $(CC) -dM -E - < /dev/null | grep __GNUC__ | cut -f3 -d\ )
CC_MIN:=$(shell $(CC) -dM -E - < /dev/null | grep __GNUC_MINOR__ | cut -f3 -d\ )
CC_PATCHLEVEL:=$(shell $(CC) -dM -E - < /dev/null | grep __GNUC_PATCHLEVEL__ | cut -f3 -d\ )
CC_VER:=$(shell echo $$(( $(CC_MAJ) * 10000 + $(CC_MIN) * 100 + $(CC_PATCHLEVEL) )))
ifeq ($(shell test $(CC_VER) -lt 40901 && echo 1),1)
  $(warning This GCC version $(CC_VER) is likely broken. Enabling relocation workaround.)
  RELOC_WORKAROUND = 1
endif

############################################################################
# Below here nothing should be changed...

# Here the Arduino variant is selected by the board type
# HARDWARE_VARIANT = "arduino", "Sanguino", "Gen7", ...
# MCU = "atmega1280", "Mega2560", "atmega2560", "atmega644p", ...

ifeq ($(HARDWARE_MOTHERBOARD),0)

  # No motherboard selected

#
# RAMPS 1.3 / 1.4 - ATmega1280, ATmega2560
#

# MEGA/RAMPS up to 1.2
else ifeq ($(HARDWARE_MOTHERBOARD),1000)

# Minitronics v1.0/1.1
else ifeq ($(HARDWARE_MOTHERBOARD),1400)
  MCU              ?= atmega1281
  PROG_MCU         ?= m1281
# Silvergate v1.0

endif

# Be sure to regenerate speed_lookuptable.h with create_speed_lookuptable.py
# if you are setting this to something other than 16MHz
# Do not put the UL suffix, it's done later on.
# Set to 16Mhz if not yet set.
F_CPU ?= 16000000

# Set to microcontroller if IS_MCU not yet set
IS_MCU ?= 1

ifeq ($(IS_MCU),1)
  # Set to arduino, ATmega2560 if not yet set.
  HARDWARE_VARIANT ?= arduino
  MCU              ?= atmega2560
  PROG_MCU         ?= m2560

  TOOL_PREFIX = avr
  MCU_FLAGS   = -mmcu=$(MCU)
  SIZE_FLAGS  = --mcu=$(MCU) -C
else
  TOOL_PREFIX = arm-none-eabi
  CPU_FLAGS   = -mthumb -mcpu=$(MCPU)
  SIZE_FLAGS  = -A
endif

# Arduino contained the main source code for the Arduino
# Libraries, the "hardware variant" are for boards
# that derives from that, and their source are present in
# the main Marlin source directory

TARGET = $(notdir $(CURDIR))

# VPATH tells make to look into these directory for source files,
# there is no need to specify explicit pathnames as long as the
# directory is added here

# The Makefile for previous versions of Marlin used VPATH for all
# source files, but for Marlin 2.0, we use VPATH only for arduino
# library files.

VPATH = .
VPATH += $(BUILD_DIR)
VPATH += $(HARDWARE_SRC)

ifeq ($(HARDWARE_VARIANT), $(filter $(HARDWARE_VARIANT),arduino Teensy Sanguino))
  # Old libraries (avr-core 1.6.21 < / Arduino < 1.6.8)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/SPI
  # New libraries (avr-core >= 1.6.21 / Arduino >= 1.6.8)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/SPI/src
endif

ifeq ($(IS_MCU),1)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/cores/arduino

  # Old libraries (avr-core 1.6.21 < / Arduino < 1.6.8)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/SPI
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/SoftwareSerial
  # New libraries (avr-core >= 1.6.21 / Arduino >= 1.6.8)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/SPI/src
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/SoftwareSerial/src
endif

VPATH += $(ARDUINO_INSTALL_DIR)/libraries/LiquidCrystal/src

ifeq ($(LIQUID_TWI2), 1)
  WIRE   = 1
  VPATH += $(ARDUINO_INSTALL_DIR)/libraries/LiquidTWI2
endif
ifeq ($(WIRE), 1)
  # Old libraries (avr-core 1.6.21 / Arduino < 1.6.8)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/Wire
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/Wire/utility
  # New libraries (avr-core >= 1.6.21 / Arduino >= 1.6.8)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/Wire/src
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/libraries/Wire/src/utility
endif
ifeq ($(NEOPIXEL), 1)
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/Adafruit_NeoPixel
endif
ifeq ($(U8GLIB), 1)
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/U8glib
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/U8glib/csrc
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/U8glib/cppsrc
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/U8glib/fntsrc
endif
ifeq ($(TMC), 1)
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/TMCStepper/src
VPATH += $(ARDUINO_INSTALL_DIR)/libraries/TMCStepper/src/source
endif

ifeq ($(HARDWARE_VARIANT), arduino)
  HARDWARE_SUB_VARIANT ?= mega
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/arduino/avr/variants/$(HARDWARE_SUB_VARIANT)
else ifeq ($(HARDWARE_VARIANT), Sanguino)
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/marlin/avr/variants/sanguino
else ifeq ($(HARDWARE_VARIANT), archim)
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/system/libsam
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/system/CMSIS/CMSIS/Include/
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/system/CMSIS/Device/ATMEL/
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/cores/arduino
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/cores/arduino/avr
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/cores/arduino/USB
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/libraries/Wire/src
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/libraries/SPI/src
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/libraries/U8glib/src/clib
  VPATH   += $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/variants/archim
  LDSCRIPT = $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/variants/archim/linker_scripts/gcc/flash.ld
  LDLIBS   = $(ARDUINO_INSTALL_DIR)/packages/ultimachine/hardware/sam/1.6.9-b/variants/archim/libsam_sam3x8e_gcc_rel.a
else
  HARDWARE_SUB_VARIANT ?= standard
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/$(HARDWARE_VARIANT)/variants/$(HARDWARE_SUB_VARIANT)
endif

LIB_SRC = wiring.c \
  wiring_analog.c wiring_digital.c \
  wiring_shift.c WInterrupts.c hooks.c

ifeq ($(HARDWARE_VARIANT), archim)
  LIB_ASRC += wiring_pulse_asm.S
else
  LIB_SRC += wiring_pulse.c
endif

ifeq ($(HARDWARE_VARIANT), Teensy)
  LIB_SRC = wiring.c
  VPATH += $(ARDUINO_INSTALL_DIR)/hardware/teensy/cores/teensy
endif

LIB_CXXSRC = WMath.cpp WString.cpp Print.cpp SPI.cpp

ifeq ($(NEOPIXEL), 1)
  LIB_CXXSRC += Adafruit_NeoPixel.cpp
endif

ifeq ($(LIQUID_TWI2), 0)
  LIB_CXXSRC += LiquidCrystal.cpp
else
  LIB_SRC += twi.c
  LIB_CXXSRC += Wire.cpp LiquidTWI2.cpp
endif

ifeq ($(WIRE), 1)
  LIB_SRC += twi.c
  LIB_CXXSRC +	= Wire.cpp
endif

ifeq ($(TONE), 1)
  LIB_CXXSRC += Tone.cpp
endif

ifeq ($(U8GLIB), 1)
  LIB_CXXSRC += U8glib.cpp
  LIB_SRC += u8g_ll_api.c u8g_bitmap.c u8g_clip.c u8g_com_null.c u8g_delay.c \
    u8g_page.c u8g_pb.c u8g_pb16h1.c u8g_rect.c u8g_state.c u8g_font.c \
    u8g_font_6x13.c u8g_font_04b_03.c u8g_font_5x8.c
endif

ifeq ($(TMC), 1)
  LIB_CXXSRC += TMCStepper.cpp COOLCONF.cpp DRV_STATUS.cpp IHOLD_IRUN.cpp \
    CHOPCONF.cpp GCONF.cpp PWMCONF.cpp DRV_CONF.cpp DRVCONF.cpp DRVCTRL.cpp \
    DRVSTATUS.cpp ENCMODE.cpp RAMP_STAT.cpp SGCSCONF.cpp SHORT_CONF.cpp \
    SMARTEN.cpp SW_MODE.cpp SW_SPI.cpp TMC2130Stepper.cpp TMC2208Stepper.cpp \
    TMC2209Stepper.cpp TMC2660Stepper.cpp TMC5130Stepper.cpp TMC5160Stepper.cpp
endif

ifeq ($(RELOC_WORKAROUND), 1)
  LD_PREFIX=-nodefaultlibs
  LD_SUFFIX=-lm -lgcc -lc -lgcc
endif

#Check for Arduino 1.0.0 or higher and use the correct source files for that version
ifeq ($(shell [ $(ARDUINO_VERSION) -ge 100 ] && echo true), true)
  LIB_CXXSRC += main.cpp
else
  LIB_SRC += pins_arduino.c main.c
endif

FORMAT = ihex

# Name of this Makefile (used for "make depend").
MAKEFILE = Makefile

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = stabs

OPT = s

DEFINES ?=

# Program settings
CC = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-gcc
CXX = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-g++
OBJCOPY = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-objcopy
OBJDUMP = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-objdump
AR  = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-ar
SIZE = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-size
NM = $(AVR_TOOLS_PATH)$(TOOL_PREFIX)-nm
AVRDUDE = avrdude
REMOVE = rm -f
MV = mv -f

# Place -D or -U options here
CDEFS    = -DF_CPU=$(F_CPU)UL ${addprefix -D , $(DEFINES)} -DARDUINO=$(ARDUINO_VERSION)
CXXDEFS  = $(CDEFS)

ifeq ($(HARDWARE_VARIANT), Teensy)
  CDEFS      += -DUSB_SERIAL
  LIB_SRC    += usb.c pins_teensy.c
  LIB_CXXSRC += usb_api.cpp

else ifeq ($(HARDWARE_VARIANT), archim)
  CDEFS      += -DARDUINO_SAM_ARCHIM -DARDUINO_ARCH_SAM -D__SAM3X8E__
  CDEFS      += -DUSB_VID=0x27B1 -DUSB_PID=0x0001 -DUSBCON
  CDEFS      += '-DUSB_MANUFACTURER="UltiMachine"' '-DUSB_PRODUCT_STRING="Archim"'

  LIB_CXXSRC += variant.cpp IPAddress.cpp Reset.cpp RingBuffer.cpp Stream.cpp \
    UARTClass.cpp  USARTClass.cpp abi.cpp new.cpp watchdog.cpp CDC.cpp \
    PluggableUSB.cpp USBCore.cpp

  LIB_SRC    += cortex_handlers.c iar_calls_sam3.c syscalls_sam3.c dtostrf.c itoa.c

  ifeq ($(U8GLIB), 1)
    LIB_SRC += u8g_com_api.c u8g_pb32h1.c
  endif
endif

# Add all the source directories as include directories too
CINCS = ${addprefix -I ,${VPATH}}
CXXINCS = ${addprefix -I ,${VPATH}}

# Silence warnings for library code (won't work for .h files, unfortunately)
LIBWARN = -w -Wno-packed-bitfield-compat

# Compiler flag to set the C/CPP Standard level.
CSTANDARD = -std=gnu99
CXXSTANDARD = -std=gnu++11
CDEBUG = -g$(DEBUG)
CWARN   = -Wall -Wstrict-prototypes -Wno-packed-bitfield-compat -Wno-pragmas -Wunused-parameter
CXXWARN = -Wall                     -Wno-packed-bitfield-compat -Wno-pragmas -Wunused-parameter
CTUNING = -fsigned-char -funsigned-bitfields -fno-exceptions \
          -fshort-enums -ffunction-sections -fdata-sections
ifneq ($(HARDWARE_MOTHERBOARD),)
  CTUNING += -DMOTHERBOARD=${HARDWARE_MOTHERBOARD}
endif

#CEXTRA = -Wa,-adhlns=$(<:.c=.lst)
CXXEXTRA = -fno-use-cxa-atexit -fno-threadsafe-statics -fno-rtti
CFLAGS := $(CDEBUG) $(CDEFS) $(CINCS) -O$(OPT) $(CEXTRA)   $(CTUNING) $(CSTANDARD)
CXXFLAGS :=         $(CDEFS) $(CINCS) -O$(OPT) $(CXXEXTRA) $(CTUNING) $(CXXSTANDARD)
ASFLAGS :=          $(CDEFS)
#ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs

ifeq ($(HARDWARE_VARIANT), archim)
  LD_PREFIX = -Wl,--gc-sections,-Map,Marlin.ino.map,--cref,--check-sections,--entry=Reset_Handler,--unresolved-symbols=report-all,--warn-common,--warn-section-align
  LD_SUFFIX = $(LDLIBS)

  LDFLAGS   = -lm -T$(LDSCRIPT) -u _sbrk -u link -u _close -u _fstat -u _isatty
  LDFLAGS  += -u _lseek -u _read -u _write -u _exit -u kill -u _getpid
else
  LD_PREFIX = -Wl,--gc-sections,--relax
  LDFLAGS   = -lm
  CTUNING   += -flto
endif

# Programming support using avrdude. Settings and variables.
AVRDUDE_PORT = $(UPLOAD_PORT)
AVRDUDE_WRITE_FLASH = -Uflash:w:$(BUILD_DIR)/$(TARGET).hex:i
ifeq ($(shell uname -s), Linux)
  AVRDUDE_CONF = /etc/avrdude/avrdude.conf
else
  AVRDUDE_CONF = $(ARDUINO_INSTALL_DIR)/hardware/tools/avr/etc/avrdude.conf
endif
AVRDUDE_FLAGS = -D -C$(AVRDUDE_CONF) \
  -p$(PROG_MCU) -P$(AVRDUDE_PORT) -c$(AVRDUDE_PROGRAMMER) \
  -b$(UPLOAD_RATE)

# Since Marlin 2.0, the source files may be distributed into several
# different directories, so it is necessary to find them recursively

SRC    = $(shell find src -name '*.c'   -type f)
CXXSRC = $(shell find src -name '*.cpp' -type f)

# Define all object files.
OBJ  = ${patsubst %.c,   $(BUILD_DIR)/arduino/%.o, ${LIB_SRC}}
OBJ += ${patsubst %.cpp, $(BUILD_DIR)/arduino/%.o, ${LIB_CXXSRC}}
OBJ += ${patsubst %.S,   $(BUILD_DIR)/arduino/%.o, ${LIB_ASRC}}
OBJ += ${patsubst %.c,   $(BUILD_DIR)/%.o, ${SRC}}
OBJ += ${patsubst %.cpp, $(BUILD_DIR)/%.o, ${CXXSRC}}

# Define all listing files.
LST = $(LIB_ASRC:.S=.lst) $(LIB_CXXSRC:.cpp=.lst) $(LIB_SRC:.c=.lst)

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS   = $(MCU_FLAGS) $(CPU_FLAGS) $(CFLAGS) -I.
ALL_CXXFLAGS = $(MCU_FLAGS) $(CPU_FLAGS) $(CXXFLAGS)
ALL_ASFLAGS  = $(MCU_FLAGS) $(CPU_FLAGS) $(ASFLAGS) -x assembler-with-cpp

# set V=1 (eg, "make V=1") to print the full commands etc.
ifneq ($V,1)
  Pecho=@echo
  P=@
else
  Pecho=@:
  P=
endif

# Create required build hierarchy if it does not exist

$(shell mkdir -p $(dir $(OBJ)))

# Default target.
all: sizeafter

build: elf hex bin

elf: $(BUILD_DIR)/$(TARGET).elf
bin: $(BUILD_DIR)/$(TARGET).bin
hex: $(BUILD_DIR)/$(TARGET).hex
eep: $(BUILD_DIR)/$(TARGET).eep
lss: $(BUILD_DIR)/$(TARGET).lss
sym: $(BUILD_DIR)/$(TARGET).sym

# Program the device.
# Do not try to reset an Arduino if it's not one
upload: $(BUILD_DIR)/$(TARGET).hex
ifeq (${AVRDUDE_PROGRAMMER}, arduino)
	stty hup < $(UPLOAD_PORT); true
endif
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)
ifeq (${AVRDUDE_PROGRAMMER}, arduino)
	stty -hup < $(UPLOAD_PORT); true
endif

# Display size of file.
HEXSIZE = $(SIZE) --target=$(FORMAT) $(BUILD_DIR)/$(TARGET).hex
ELFSIZE = $(SIZE)  $(SIZE_FLAGS) $(BUILD_DIR)/$(TARGET).elf; \
          $(SIZE)  $(BUILD_DIR)/$(TARGET).elf
sizebefore:
	$P if [ -f $(BUILD_DIR)/$(TARGET).elf ]; then echo; echo $(MSG_SIZE_BEFORE); $(HEXSIZE); echo; fi

sizeafter: build
	$P if [ -f $(BUILD_DIR)/$(TARGET).elf ]; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); echo; fi


# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
  --change-section-address .data-0x800000 \
  --change-section-address .bss-0x800000 \
  --change-section-address .noinit-0x800000 \
  --change-section-address .eeprom-0x810000


coff: $(BUILD_DIR)/$(TARGET).elf
	$(COFFCONVERT) -O coff-avr $(BUILD_DIR)/$(TARGET).elf $(TARGET).cof


extcoff: $(TARGET).elf
	$(COFFCONVERT) -O coff-ext-avr $(BUILD_DIR)/$(TARGET).elf $(TARGET).cof


.SUFFIXES: .elf .hex .eep .lss .sym .bin
.PRECIOUS: .o

.elf.hex:
	$(Pecho) "  COPY  $@"
	$P $(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

.elf.bin:
	$(Pecho) "  COPY  $@"
	$P $(OBJCOPY) -O binary -R .eeprom $< $@

.elf.eep:
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	  --change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
.elf.lss:
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
.elf.sym:
	$(NM) -n $< > $@

# Link: create ELF output file from library.

$(BUILD_DIR)/$(TARGET).elf: $(OBJ) Configuration.h
	$(Pecho) "  CXX   $@"
	$P $(CXX) $(LD_PREFIX) $(ALL_CXXFLAGS) -o $@ -L. $(OBJ) $(LDFLAGS) $(LD_SUFFIX)

# Object files that were found in "src" will be stored in $(BUILD_DIR)
# in directories that mirror the structure of "src"

$(BUILD_DIR)/%.o: %.c Configuration.h Configuration_adv.h $(MAKEFILE)
	$(Pecho) "  CC    $<"
	$P $(CC) -MMD -c $(ALL_CFLAGS) $(CWARN) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Configuration.h Configuration_adv.h $(MAKEFILE)
	$(Pecho) "  CXX   $<"
	$P $(CXX) -MMD -c $(ALL_CXXFLAGS) $(CXXWARN) $< -o $@

# Object files for Arduino libs will be created in $(BUILD_DIR)/arduino

$(BUILD_DIR)/arduino/%.o: %.c Configuration.h Configuration_adv.h $(MAKEFILE)
	$(Pecho) "  CC    $<"
	$P $(CC) -MMD -c $(ALL_CFLAGS) $(LIBWARN) $< -o $@

$(BUILD_DIR)/arduino/%.o: %.cpp Configuration.h Configuration_adv.h $(MAKEFILE)
	$(Pecho) "  CXX   $<"
	$P $(CXX) -MMD -c $(ALL_CXXFLAGS)  $(LIBWARN) $< -o $@

$(BUILD_DIR)/arduino/%.o: %.S $(MAKEFILE)
	$(Pecho) "  CXX   $<"
	$P $(CXX) -MMD -c $(ALL_ASFLAGS) $< -o $@

# Target: clean project.
clean:
	$(Pecho) "  RMDIR $(BUILD_DIR)/"
	$P rm -rf $(BUILD_DIR)


.PHONY: all build elf hex eep lss sym program coff extcoff clean depend sizebefore sizeafter

# Automatically include the dependency files created by gcc
-include ${patsubst %.o, %.d, ${OBJ}}
