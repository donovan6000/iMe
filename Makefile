# Program name
PROG = iMe.elf

# Tool locations
CC = /opt/avr-toolchain/bin/avr-gcc
COPY = /opt/avr-toolchain/bin/avr-objcopy
M3DLINUX = /usr/sbin/m3d-linux

# C source files
CSRCS = main.c \
	ASF/common/boards/user_board/init.c \
	ASF/common/services/clock/xmega/sysclk.c \
	ASF/common/services/sleepmgr/xmega/sleepmgr.c \
	ASF/common/services/usb/class/cdc/device/udi_cdc.c \
	ASF/common/services/usb/class/cdc/device/udi_cdc_desc.c \
	ASF/common/services/usb/udc/udc.c \
	ASF/xmega/drivers/nvm/nvm.c \
	ASF/xmega/drivers/usb/usb_device.c

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
CFLAGS = -Wall -Os -std=gnu99 -D BOARD=USER_BOARD -mmcu=atxmega32a4u -Wl,--section-start=.BOOT=0x8000 -ffunction-sections -fdata-sections -Wl,--gc-sections -mrelax -funsigned-char -fno-strict-aliasing

# Make all
all: $(CSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(CFLAGS) -x assembler-with-cpp -D__ASSEMBLY__ -c $(ASSRCS) $(LIBS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(CFLAGS) -o $(PROG) $(CSRCS) *.o $(LIBS)
	@rm -f *.o
	@$(COPY) -O binary $(PROG) $(shell date +"%Y%m%d%H").hex
	@echo *.hex is ready

# Make clean
clean:
	rm -f $(PROG) *.hex *.o

# Make run
run:
	@$(M3DLINUX) -a -x -r $(wildcard *.hex)
