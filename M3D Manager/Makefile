# Target platform options: WINDOWS, OSX, LINUX
PROGRAM_NAME = m3d-manager
TARGET_PLATFORM = LINUX

SRCS = main.cpp printer.cpp gcode.cpp
CFLAGS = -Wall -std=c++11 -O3

ifeq ($(TARGET_PLATFORM), WINDOWS)
	EXECUTABLE = $(PROGRAM_NAME).exe
	CC = i686-w64-mingw32-g++
	CFLAGS += -static-libgcc -static-libstdc++
endif

ifeq ($(TARGET_PLATFORM), OSX)
	EXECUTABLE = $(PROGRAM_NAME)
	CC = clang++
	CFLAGS += -stdlib=libc++ -m32
endif

ifeq ($(TARGET_PLATFORM), LINUX)
	EXECUTABLE = $(PROGRAM_NAME)
	CC = g++
	CFLAGS += -static-libgcc -static-libstdc++ -m32
endif

all: $(EXECUTABLE)

$(EXECUTABLE):
	$(CC) $(CFLAGS) bin2h.cpp -o bin2h
	./bin2h "../iMe 1900000001.hex"
	./bin2h "drivers/Linux/90-m3d-local.rules"
	./bin2h "drivers/Windows/M3D.cat"
	./bin2h "drivers/Windows/M3D.inf"
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SRCS) $(LIBS) -D $(TARGET_PLATFORM)
	rm -f ./bin2h "./iMe 1900000001_hex.h" "./_90_m3d_local_rules.h" "./M3D_cat.h" "./M3D_inf.h"

clean:
	rm -f $(EXECUTABLE) ./bin2h "./iMe 1900000001_hex.h" "./_90_m3d_local_rules.h" "./M3D_cat.h" "./M3D_inf.h"

run:
	./$(EXECUTABLE)

install:
	install $(EXECUTABLE) /usr/sbin/$(EXECUTABLE)
