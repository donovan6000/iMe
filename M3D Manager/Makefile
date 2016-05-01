# Target platform options: WINDOWS32, WINDOWS64, OSX32, OSX64, LINUX32, LINUX64
PROGRAM_NAME = M3D\ Manager
VERSION = 0.01
TARGET_PLATFORM = LINUX64
USE_GUI = TRUE

SRCS = main.cpp printer.cpp gcode.cpp
CFLAGS = -Wall -std=c++11 -O3 -D VERSION=$(VERSION)

ifeq ($(TARGET_PLATFORM), WINDOWS32)
	PLATFORM = WINDOWS
	EXECUTABLE = $(PROGRAM_NAME).exe
	CC = i686-w64-mingw32-g++
	CFLAGS += -static-libgcc -static-libstdc++ -lsetupapi
	ARCHIVE = "M3D Manager Windows.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
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
	ARCHIVE = "M3D Manager Windows.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		CFLAGS += -D USE_GUI `/usr/x86_64-w64-mingw32/bin/wx-config --cxxflags --libs --static`
	endif
	SRCS += resource.o
	WINDRES = x86_64-w64-mingw32-windres
endif

ifeq ($(TARGET_PLATFORM), OSX32)
	PLATFORM = OSX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = clang++
	CFLAGS += -stdlib=libc++ -m32
	ARCHIVE = "M3D Manager OS X.zip"
	BUNDLE = "M3D Manager.app"
	ifeq ($(USE_GUI), TRUE)
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

ifeq ($(TARGET_PLATFORM), OSX64)
	PLATFORM = OSX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = clang++
	CFLAGS += -stdlib=libc++ -m64
	ARCHIVE = "M3D Manager OS X.zip"
	BUNDLE = "M3D Manager.app"
	ifeq ($(USE_GUI), TRUE)
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

ifeq ($(TARGET_PLATFORM), LINUX32)
	PLATFORM = LINUX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = g++
	CFLAGS += -pthread -static-libgcc -static-libstdc++ -m32
	ARCHIVE = "M3D Manager Linux.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

ifeq ($(TARGET_PLATFORM), LINUX64)
	PLATFORM = LINUX
	EXECUTABLE = $(PROGRAM_NAME)
	CC = g++
	CFLAGS += -pthread -static-libgcc -static-libstdc++ -m64
	ARCHIVE = "M3D Manager Linux.zip"
	BUNDLE = $(EXECUTABLE)
	ifeq ($(USE_GUI), TRUE)
		CFLAGS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
endif

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRCS)
	$(CC) $(CFLAGS) "bin2h.cpp" -o "bin2h"
	./bin2h "../iMe 1900000001.hex"
	./bin2h "resources/Linux/90-m3d-local.rules"
	./bin2h "resources/Windows/M3D.cat"
	./bin2h "resources/Windows/M3D.inf"
	$(CC) -o $(EXECUTABLE) $(SRCS) $(LIBS) -D $(PLATFORM) $(CFLAGS)
	rm -f "resource.o" "bin2h" "iMe 1900000001_hex.h" "_90_m3d_local_rules.h" "M3D_cat.h" "M3D_inf.h"

clean:
	rm -rf $(EXECUTABLE) $(BUNDLE) $(ARCHIVE) "resource.o" "applet.iconset" "icon.ico" "bin2h" "iMe 1900000001_hex.h" "_90_m3d_local_rules.h" "M3D_cat.h" "M3D_inf.h"

run:
	./$(EXECUTABLE)

install:
	install $(EXECUTABLE) /usr/sbin/$(EXECUTABLE)

resourceFile:
	rm -f "resource.o" "icon.ico"
	convert "resources/icon.png" -define icon:auto-resize="256,128,64,48,32,24,16" "icon.ico"
	$(WINDRES) "resources/Windows/resource.rc" -o "resource.o"
	rm -f "icon.ico"

app:
	rm -rf "M3D Manager.app" "applet.iconset"
	cp -r "resources/OS X/M3D Manager.app" .
	mv $(EXECUTABLE) "M3D Manager.app/Contents/MacOS"
	mkdir "applet.iconset"
	sips -z 16 16 "resources/icon.png" --out "applet.iconset/icon_16x16.png"
	sips -z 32 32 "resources/icon.png" --out "applet.iconset/icon_16x16@2x.png"
	sips -z 32 32 "resources/icon.png" --out "applet.iconset/icon_32x32.png"
	sips -z 64 64 "resources/icon.png" --out "applet.iconset/icon_32x32@2x.png"
	sips -z 128 128 "resources/icon.png" --out "applet.iconset/icon_128x128.png"
	sips -z 256 256 "resources/icon.png" --out "applet.iconset/icon_128x128@2x.png"
	sips -z 256 256 "resources/icon.png" --out "applet.iconset/icon_256x256.png"
	sips -z 512 512 "resources/icon.png" --out "applet.iconset/icon_256x256@2x.png"
	sips -z 512 512 "resources/icon.png" --out "applet.iconset/icon_512x512.png"
	sips -z 1024 1024 "resources/icon.png" --out "applet.iconset/icon_512x512@2x.png"
	iconutil --convert icns "applet.iconset"
	rm -rf "applet.iconset"
	mv "applet.icns" "M3D Manager.app/Contents/Resources"

package:
	rm -f $(ARCHIVE)
	zip -9 -r $(ARCHIVE) $(BUNDLE)
	rm -rf $(BUNDLE)
