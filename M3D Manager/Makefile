# Target platform options: WINDOWS, OSX, LINUX
PROG = m3d-manager
TARGET_PLATFORM = LINUX

SRCS = main.cpp printer.cpp gcode.cpp
CFLAGS = -Wall -std=c++11 -O3

ifeq ($(TARGET_PLATFORM), WINDOWS)
	PROG = $(PROG).exe
	CC = i686-w64-mingw32-g++
	CFLAGS += -static-libgcc -static-libstdc++
endif

ifeq ($(TARGET_PLATFORM), OSX)
	CC = clang++
	CFLAGS += -stdlib=libc++ -m32
endif

ifeq ($(TARGET_PLATFORM), LINUX)
	CC = g++
	CFLAGS += -static-libgcc -static-libstdc++ -m32
endif

all: $(PROG)

$(PROG):
	$(CC) bin2h.cpp -o bin2h
	./bin2h "../iMe 1900000001.hex"
	./bin2h "drivers/Linux/90-m3d-local.rules"
	./bin2h "drivers/Windows/M3D.cat"
	./bin2h "drivers/Windows/M3D.inf"
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS) -D $(TARGET_PLATFORM)
	rm -f ./bin2h "./iMe 1900000001_hex.h" "./_90_m3d_local_rules.h" "./M3D_cat.h" "./M3D_inf.h"

clean:
	rm -f $(PROG) ./bin2h "./iMe 1900000001_hex.h" "./_90_m3d_local_rules.h" "./M3D_cat.h" "./M3D_inf.h"

run:
	./$(PROG)

install:
	install $(PROG) /usr/sbin/$(PROG)
