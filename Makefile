# Firmware name and version
NAME = "iMe"
FIRMWARE_VERSION = 00.00.01.25
ROM_VERSION_STRING = 1900000125

ENABLE_DEBUG_FEATURES = false
ENABLE_USELESS_COMMANDS = false
ENABLE_HOST_COMMANDS = false
STORE_CONSTANTS_IN_PROGRAM_SPACE = false
REGULATE_EXTRUDER_CURRENT = false
ENABLE_BOUNDARY_ENFORCING = true
ENABLE_BACKLASH_COMPENSATION = true
ENABLE_BED_LEVELING_COMPENSATION = true
ENABLE_SKEW_COMPENSATION = true

FIRMWARE_NAME = $(subst $\",,$(NAME))

# Tool locations
ifeq ($(OS), Windows_NT)
	CC = "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe"
	COPY = "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objcopy.exe"
	SIZE = "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-size.exe"
	DUMP = "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objdump.exe"
	M33MANAGER = "M33 Manager\M33 Manager.exe"
	
	# Linker time optimizations make the firmware larger when using the Windows compiler suite
	FLAGS = -fno-lto
else
	CC = /opt/avr-toolchain/bin/avr-gcc
	COPY = /opt/avr-toolchain/bin/avr-objcopy
	SIZE = /opt/avr-toolchain/bin/avr-size
	DUMP = /opt/avr-toolchain/bin/avr-objdump
	M33MANAGER = "M33 Manager/M33 Manager"
	
	# Linker time optimizations work correctly when using the Linux compiler suite
	FLAGS = -flto -flto-partition=1to1
endif

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
	src/ASF/xmega/drivers/crc/crc.c \
	src/ASF/xmega/drivers/nvm/nvm.c \
	src/ASF/xmega/drivers/tc/tc.c \
	src/ASF/xmega/drivers/twi/twim.c \
	src/ASF/xmega/drivers/twi/twis.c \
	src/ASF/xmega/drivers/usb/usb_device.c

# C++ source files
CPPSRCS = accelerometer.cpp \
	common.cpp \
	fan.cpp \
	gcode.cpp \
	heater.cpp \
	led.cpp \
	main.cpp \
	motors.cpp \
	vector.cpp

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
	src/ASF/xmega/drivers/crc \
	src/ASF/xmega/drivers/nvm \
	src/ASF/xmega/drivers/pmic \
	src/ASF/xmega/drivers/sleep \
	src/ASF/xmega/drivers/tc \
	src/ASF/xmega/drivers/twi \
	src/ASF/xmega/drivers/usb \
	src/ASF/xmega/services \
	src/ASF/xmega/utils \
	src/ASF/xmega/utils/assembler \
	src/ASF/xmega/utils/bit_handling \
	src/ASF/xmega/utils/preprocessor \
	src/config

# Compiler flags
FLAGS += -D BOARD=USER_BOARD -D FIRMWARE_NAME="$(FIRMWARE_NAME)" -D FIRMWARE_VERSION=$(FIRMWARE_VERSION) -D ENABLE_DEBUG_FEATURES=$(ENABLE_DEBUG_FEATURES) -D ENABLE_USELESS_COMMANDS=$(ENABLE_USELESS_COMMANDS) -D ENABLE_HOST_COMMANDS=$(ENABLE_HOST_COMMANDS) -D STORE_CONSTANTS_IN_PROGRAM_SPACE=$(STORE_CONSTANTS_IN_PROGRAM_SPACE) -D REGULATE_EXTRUDER_CURRENT=$(REGULATE_EXTRUDER_CURRENT) -D ENABLE_BOUNDARY_ENFORCING=$(ENABLE_BOUNDARY_ENFORCING) -D ENABLE_BACKLASH_COMPENSATION=$(ENABLE_BACKLASH_COMPENSATION) -D ENABLE_BED_LEVELING_COMPENSATION=$(ENABLE_BED_LEVELING_COMPENSATION) -D ENABLE_SKEW_COMPENSATION=$(ENABLE_SKEW_COMPENSATION) -Os -finput-charset=UTF-8 -fexec-charset=UTF-8 -mmcu=atxmega32c4 -Wall -Wno-maybe-uninitialized -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -fno-strict-aliasing -Werror-implicit-function-declaration -Wpointer-arith -mcall-prologues -mstrict-X -maccumulate-args -fno-tree-ter -mrelax
ASFLAGS = -std=c++14 -x assembler-with-cpp
CFLAGS = -std=gnu99 -x c -Wstrict-prototypes
CPPFLAGS = -std=c++14 -x c++
LFLAGS = -Wl,--start-group -Wl,--end-group -Wl,--gc-sections

# Make - Compiles firmware
all:
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)),-I $(INC)) $(FLAGS) $(ASFLAGS) -c $(ASSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)),-I $(INC)) $(FLAGS) $(CFLAGS) -c $(CSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)),-I $(INC)) $(FLAGS) $(CPPFLAGS) -c $(CPPSRCS)
	$(CC) $(foreach INC, $(addprefix , $(INCPATH)),-I $(INC)) $(FLAGS) $(LFLAGS) *.o -o "$(FIRMWARE_NAME).elf"
	@$(COPY) -S -O binary "$(FIRMWARE_NAME).elf" "$(FIRMWARE_NAME) $(ROM_VERSION_STRING).hex"
	@$(SIZE) --mcu=atxmega32c4 -C "$(FIRMWARE_NAME).elf"
	@rm -f *.o "$(FIRMWARE_NAME).elf"
	@echo $(FIRMWARE_NAME) $(ROM_VERSION_STRING).hex is ready

# Make clean - Removes temporary files and compiled firmware
clean:
	rm -f "$(FIRMWARE_NAME).elf" "$(FIRMWARE_NAME) $(ROM_VERSION_STRING).hex" *.o

# Make run - Flashes and runs compiled firmware
run:
	@$(M33MANAGER) -r "$(FIRMWARE_NAME) $(ROM_VERSION_STRING).hex"
	@$(M33MANAGER) -f
