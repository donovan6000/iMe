# Target platform options: WINDOWS32, WINDOWS64, OSX32, OSX64, LINUX32, LINUX64
PROGRAM_NAME = M33\ Manager
VERSION = 0.08
TARGET_PLATFORM = LINUX64
USE_GUI = TRUE
IME_ROM_VERSION_STRING = 1900000103
M3D_ROM_VERSION_STRING = 2016040401

SRCS = main.cpp printer.cpp gcode.cpp common.cpp
CFLAGS = -Wall -std=c++11 -O3 -D VERSION=$(VERSION) -D IME_ROM_VERSION_STRING=$(IME_ROM_VERSION_STRING) -D IME_HEADER="iMe $(IME_ROM_VERSION_STRING)_hex.h" -D IME_HEX_SIZE=iMe$(IME_ROM_VERSION_STRING)_hexSize -D IME_HEX_DATA=iMe$(IME_ROM_VERSION_STRING)_hexData -D M3D_ROM_VERSION_STRING=$(M3D_ROM_VERSION_STRING) -D M3D_HEADER="M3D $(M3D_ROM_VERSION_STRING)_hex.h" -D M3D_HEX_SIZE=m3D$(M3D_ROM_VERSION_STRING)_hexSize -D M3D_HEX_DATA=m3D$(M3D_ROM_VERSION_STRING)_hexData

ifeq ($(TARGET_PLATFORM), WINDOWS32)
	PLATFORM = WINDOWS
	EXECUTABLE = $(PROGRAM_NAME).exe
	CC = i686-w64-mingw32-g++
	CFLAGS += -static-libgcc -static-libstdc++ -lsetupapi
	ARCHIVE = "M33 Manager Windows.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		CFLAGS += -D USE_GUI `/usr/i686-w64-mingw32/bin/wx-config --cxxflags --libs --static`
	endif
	SRCS += resource.o
	WINDRES = i686-w64-mingw32-windres
endif

ifeq ($(TARGET_PLATFORM), WINDOWS64)
	PLATFORM = WINDOWS
	EXECUTABLE = $(PROGRAM_NAME).exe
	CC = x86_64-w64-mingw32-g++
	CFLAGS += -static-libgcc -static-libstdc++ -lsetupapi
	ARCHIVE = "M33 Manager Windows.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		CFLAGS += -D USE_GUI `/usr/x86_64-w64-mingw32/bin/wx-config --cxxflags --libs --static`
	endif
	SRCS += resource.o
	WINDRES = x86_64-w64-mingw32-windres
endif

ifeq ($(TARGET_PLATFORM), OSX32)
	PLATFORM = OSX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = clang++
	CFLAGS += -framework IOKit -framework CoreFoundation -stdlib=libc++ -m32
	ARCHIVE = "M33 Manager OS X.zip"
	BUNDLE = "M33 Manager.app"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

ifeq ($(TARGET_PLATFORM), OSX64)
	PLATFORM = OSX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = clang++
	CFLAGS += -framework IOKit -framework CoreFoundation -stdlib=libc++ -m64
	ARCHIVE = "M33 Manager OS X.zip"
	BUNDLE = "M33 Manager.app"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

ifeq ($(TARGET_PLATFORM), LINUX32)
	PLATFORM = LINUX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = g++
	CFLAGS += -pthread -static-libgcc -static-libstdc++ -m32
	ARCHIVE = "M33 Manager Linux.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

ifeq ($(TARGET_PLATFORM), LINUX64)
	PLATFORM = LINUX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = g++
	CFLAGS += -pthread -static-libgcc -static-libstdc++ -m64
	ARCHIVE = "M33 Manager Linux.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

all:
	$(CC) $(CFLAGS) "bin2h.cpp" -o "bin2h"
	./bin2h "resources/files/icon.png"
	./bin2h "resources/files/refresh.png"
	./bin2h "../iMe $(IME_ROM_VERSION_STRING).hex"
	./bin2h "resources/files/M3D $(M3D_ROM_VERSION_STRING).hex"
	./bin2h "resources/Linux/90-micro-3d-local.rules"
	./bin2h "resources/Windows/M3D.cat"
	./bin2h "resources/Windows/M3D.inf"
	$(CC) -o $(EXECUTABLE) $(SRCS) $(LIBS) -D $(PLATFORM) $(CFLAGS)
	rm -f "resource.o" "bin2h" "icon_png.h" "refresh_png.h" "iMe $(IME_ROM_VERSION_STRING)_hex.h" "M3D $(M3D_ROM_VERSION_STRING)_hex.h" "_90_micro_3d_local_rules.h" "M3D_cat.h" "M3D_inf.h"

clean:
	rm -rf $(EXECUTABLE) $(BUNDLE) $(ARCHIVE) "resource.o" "applet.iconset" "icon.ico" "bin2h" "icon_png.h" "refresh_png.h" "iMe $(IME_ROM_VERSION_STRING)_hex.h" "M3D $(M3D_ROM_VERSION_STRING)_hex.h" "_90_micro_3d_local_rules.h" "M3D_cat.h" "M3D_inf.h"

run:
	./$(EXECUTABLE)

install:
	install $(EXECUTABLE) /usr/sbin/$(EXECUTABLE)

resourceFile:
	rm -f "resource.o" "icon.ico"
	convert "resources/files/icon.png" -define icon:auto-resize="256,128,64,48,32,24,16" "icon.ico"
	$(WINDRES) "resources/Windows/resource.rc" -o "resource.o"
	rm -f "icon.ico"

app:
	rm -rf "M33 Manager.app" "applet.iconset"
	cp -r "resources/OS X/M33 Manager.app" .
	mv $(EXECUTABLE) "M33 Manager.app/Contents/MacOS"
	mkdir "applet.iconset"
	sips -z 16 16 "resources/files/icon.png" --out "applet.iconset/icon_16x16.png"
	sips -z 32 32 "resources/files/icon.png" --out "applet.iconset/icon_16x16@2x.png"
	sips -z 32 32 "resources/files/icon.png" --out "applet.iconset/icon_32x32.png"
	sips -z 64 64 "resources/files/icon.png" --out "applet.iconset/icon_32x32@2x.png"
	sips -z 128 128 "resources/files/icon.png" --out "applet.iconset/icon_128x128.png"
	sips -z 256 256 "resources/files/icon.png" --out "applet.iconset/icon_128x128@2x.png"
	sips -z 256 256 "resources/files/icon.png" --out "applet.iconset/icon_256x256.png"
	sips -z 512 512 "resources/files/icon.png" --out "applet.iconset/icon_256x256@2x.png"
	sips -z 512 512 "resources/files/icon.png" --out "applet.iconset/icon_512x512.png"
	sips -z 1024 1024 "resources/files/icon.png" --out "applet.iconset/icon_512x512@2x.png"
	iconutil --convert icns "applet.iconset"
	rm -rf "applet.iconset"
	mv "applet.icns" "M33 Manager.app/Contents/Resources"

package:
	rm -f $(ARCHIVE)
	zip -9 -r $(ARCHIVE) $(BUNDLE)
	rm -rf $(BUNDLE)
