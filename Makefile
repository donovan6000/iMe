# Firmware name and version
FIRMWARE_NAME = iMe
FIRMWARE_VERSION = 1900000001

# Tool locations
CC = /opt/avr-toolchain/bin/avr-gcc
COPY = /opt/avr-toolchain/bin/avr-objcopy
SIZE = /opt/avr-toolchain/bin/avr-size
M3DLINUX = /usr/sbin/m3d-linux

# Assembly source files
ASSRCS = src/ASF/xmega/drivers/cpu/ccp.s \
	src/ASF/xmega/drivers/nvm/nvm_asm.s

# C source files
CSRCS = src/ASF/common/boards/user_board/init.c \
	src/ASF/common/services/clock/xmega/sysclk.c \
	src/ASF/common/services/ioport/xmega/ioport_compat.c \
	src/ASF/common/services/sleepmgr/xmega/sleepmgr.c \
	src/ASF/common/services/usb/class/cdc/device/udi_cdc.c \
	src/ASF/common/services/usb/class/cdc/device/udi_cdc_desc.c \
	src/ASF/common/services/usb/udc/udc.c \
	src/ASF/xmega/drivers/adc/adc.c \
	src/ASF/xmega/drivers/adc/xmega_bcd/adc_bcd.c \
	src/ASF/xmega/drivers/nvm/nvm.c \
	src/ASF/xmega/drivers/tc/tc.c \
	src/ASF/xmega/drivers/twi/twim.c \
	src/ASF/xmega/drivers/twi/twis.c \
	src/ASF/xmega/drivers/usb/usb_device.c \
	src/ASF/xmega/services/pwm/pwm.c

# C++ source files
CPPSRCS = accelerometer.cpp \
	fan.cpp \
	gcode.cpp \
	heater.cpp \
	led.cpp \
	main.cpp \
	motors.cpp

# Include paths
INCPATH = . \
	src \
	src/ASF \
	src/ASF/common \
	src/ASF/common/boards \
	src/ASF/common/boards/user_board \
	src/ASF/common/services \
	src/ASF/common/services/clock \
	src/ASF/common/services/clock/xmega \
	src/ASF/common/services/delay \
	src/ASF/common/services/delay/xmega \
	src/ASF/common/services/ioport \
	src/ASF/common/services/ioport/xmega \
	src/ASF/common/services/sleepmgr \
	src/ASF/common/services/sleepmgr/xmega \
	src/ASF/common/services/twi \
	src/ASF/common/services/twi/xmega_twi \
	src/ASF/common/services/usb \
	src/ASF/common/services/usb/class \
	src/ASF/common/services/usb/class/cdc \
	src/ASF/common/services/usb/class/cdc/device \
	src/ASF/common/services/usb/udc \
	src/ASF/common/utils \
	src/ASF/common/utils/interrupt \
	src/ASF/common/utils/make \
	src/ASF/xmega \
	src/ASF/xmega/drivers \
	src/ASF/xmega/drivers/adc \
	src/ASF/xmega/drivers/adc/xmega_bcd \
	src/ASF/xmega/drivers/cpu \
	src/ASF/xmega/drivers/nvm \
	src/ASF/xmega/drivers/pmic \
	src/ASF/xmega/drivers/sleep \
	src/ASF/xmega/drivers/tc \
	src/ASF/xmega/drivers/twi \
	src/ASF/xmega/drivers/usb \
	src/ASF/xmega/services \
	src/ASF/xmega/services/pwm \
	src/ASF/xmega/utils \
	src/ASF/xmega/utils/assembler \
	src/ASF/xmega/utils/bit_handling \
	src/ASF/xmega/utils/preprocessor \
	src/config

# Compiler flags
FLAGS = -D BOARD=USER_BOARD -D FIRMWARE_NAME="\"$(FIRMWARE_NAME)\"" -D FIRMWARE_VERSION="\"$(FIRMWARE_VERSION)\"" -Os -mmcu=atxmega32c4 -Wall
ASFLAGS = -std=c++14 -x assembler-with-cpp
CFLAGS = -std=gnu99 -x c -fdata-sections -ffunction-sections -fpack-struct -fshort-enums -fno-strict-aliasing -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -mrelax
CPPFLAGS = -std=c++14 -x c++ -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections -fpack-struct -fshort-enums
LFLAGS = -Wl,--section-start=.BOOT=0x8000 -Wl,--start-group -Wl,--end-group -Wl,--gc-sections

# Make
all:
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(ASFLAGS) -c $(ASSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(CFLAGS) -c $(CSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(CPPFLAGS) -c $(CPPSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)), -I $(INC)) $(FLAGS) $(LFLAGS) *.o -o $(FIRMWARE_NAME).elf
	@$(COPY) -O binary $(FIRMWARE_NAME).elf "$(FIRMWARE_NAME) $(FIRMWARE_VERSION).hex"
	@$(SIZE) --mcu=atxmega32c4 -C $(FIRMWARE_NAME).elf
	@rm -f *.o $(FIRMWARE_NAME).elf
	@echo $(FIRMWARE_NAME) $(FIRMWARE_VERSION).hex is ready

# Make clean
clean:
	rm -f $(FIRMWARE_NAME).elf "$(FIRMWARE_NAME) $(FIRMWARE_VERSION).hex" *.o

# Make run
run:
	@$(M3DLINUX) -a -x -r "$(FIRMWARE_NAME) $(FIRMWARE_VERSION).hex"
