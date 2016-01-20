# Program name
PROG = iMe
VERSION = 1900000001

# Tool locations
CC = /opt/avr-toolchain/bin/avr-gcc
COPY = /opt/avr-toolchain/bin/avr-objcopy
M3DLINUX = /usr/sbin/m3d-linux

# C source files
CSRCS = ASF/common/boards/user_board/init.c \
	ASF/common/services/clock/xmega/sysclk.c \
	ASF/common/services/sleepmgr/xmega/sleepmgr.c \
	ASF/common/services/usb/class/cdc/device/udi_cdc.c \
	ASF/common/services/usb/class/cdc/device/udi_cdc_desc.c \
	ASF/common/services/usb/udc/udc.c \
	ASF/xmega/drivers/nvm/nvm.c \
	ASF/xmega/drivers/usb/usb_device.c

# C++ source files
CPPSRCS = main.cpp \
	gcode.cpp

# Assembly source files
ASSRCS = ASF/xmega/drivers/cpu/ccp.s \
	ASF/xmega/drivers/nvm/nvm_asm.s

# Include paths
INCPATH = . \
	config \
	ASF/common/boards \
	ASF/common/boards/user_board \
	ASF/common/services/clock \
	ASF/common/services/clock/xmega \
	ASF/common/services/sleepmgr \
	ASF/common/services/sleepmgr/xmega \
	ASF/common/services/usb \
	ASF/common/services/usb/class/cdc \
	ASF/common/services/usb/class/cdc/device \
	ASF/common/services/usb/udc \
	ASF/common/utils \
	ASF/common/utils/interrupt \
	ASF/xmega/drivers/cpu \
	ASF/xmega/drivers/nvm \
	ASF/xmega/drivers/sleep \
	ASF/xmega/drivers/usb \
	ASF/xmega/utils \
	ASF/xmega/utils/assembler \
	ASF/xmega/utils/bit_handling \
	ASF/xmega/utils/preprocessor

# Compiler flags
FLAGS = -Wall -Os -D VERSION="\"$(VERSION)\"" -D BOARD=USER_BOARD -mmcu=atxmega32a4u -Wl,--section-start=.BOOT=0x8000 -ffunction-sections -fdata-sections -Wl,--gc-sections -mrelax -funsigned-char -fno-strict-aliasing
CFLAGS = -std=gnu99
CPPFLAGS = -std=c++14

# Make all
all: $(CSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(CPPFLAGS) -x assembler-with-cpp -D__ASSEMBLY__ -c $(ASSRCS) $(LIBS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(CFLAGS) -c $(CSRCS) $(LIBS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(CPPFLAGS) -o $(PROG).elf $(CPPSRCS) *.o $(LIBS)
	@rm -f *.o
	@$(COPY) -O binary $(PROG).elf "iMe $(VERSION).hex"
	@echo *.hex is ready

# Make clean
clean:
	rm -f $(PROG).elf "iMe $(VERSION).hex" *.o

# Make run
run:
	@$(M3DLINUX) -a -x -r "iMe $(VERSION).hex"
