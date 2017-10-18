# Target platform options: WINDOWS32, WINDOWS64, MACOS, LINUX32, LINUX64
NAME = "M33 Manager"
VERSION = 0.17
TARGET_PLATFORM = LINUX64
USE_GUI = TRUE
IME_ROM_VERSION_STRING = 1900000126
M3D_ROM_VERSION_STRING = 2016040401

SRCS = main.cpp printer.cpp gcode.cpp common.cpp
OBJ_FILES = $(addprefix working/,$(notdir $(SRCS:.cpp=.o)))
CFLAGS = -Wall -Wno-unused-function -std=c++11 -O3 -finput-charset=UTF-8 -fexec-charset=UTF-8 -D VERSION=$(VERSION) -D IME_ROM_VERSION_STRING=$(IME_ROM_VERSION_STRING) -D IME_HEX_SIZE=iMe$(IME_ROM_VERSION_STRING)HexSize -D IME_HEX_DATA=iMe$(IME_ROM_VERSION_STRING)HexData -D M3D_ROM_VERSION_STRING=$(M3D_ROM_VERSION_STRING) -D M3D_HEX_SIZE=m3D$(M3D_ROM_VERSION_STRING)HexSize -D M3D_HEX_DATA=m3D$(M3D_ROM_VERSION_STRING)HexData
CFLAGS_GENERIC = -D PACKED -Wall -Wno-unused-function -std=c++11 -O3 -finput-charset=UTF-8 -fexec-charset=UTF-8
LIBS =
EXTRA_FILES = 
PROGRAM_NAME = $(subst $\",,$(NAME))
SHELL = /bin/bash

ifeq ($(TARGET_PLATFORM), WINDOWS32)
	PLATFORM = WINDOWS
	EXECUTABLE = "$(PROGRAM_NAME).exe"
	CC = "i686-w64-mingw32-g++"
	CCP = "g++"
	CFLAGS += -static-libgcc -static-libstdc++ -lsetupapi
	CFLAGS_GENERIC += -static-libgcc -static-libstdc++
	ARCHIVE = "$(PROGRAM_NAME) Windows.zip"
	EXTRA_FILES += "./working/resources.o"
	WINDRES = "i686-w64-mingw32-windres"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		LIBS += -D USE_GUI `/usr/i686-w64-mingw32/bin/wx-config --cxxflags --libs --static`
	endif
	STRIP = "i686-w64-mingw32-strip"
endif

ifeq ($(TARGET_PLATFORM), WINDOWS64)
	PLATFORM = WINDOWS
	EXECUTABLE = "$(PROGRAM_NAME).exe"
	CC = "x86_64-w64-mingw32-g++"
	CCP = "g++"
	CFLAGS += -static-libgcc -static-libstdc++ -lsetupapi
	CFLAGS_GENERIC += -static-libgcc -static-libstdc++
	ARCHIVE = "$(PROGRAM_NAME) Windows.zip"
	EXTRA_FILES += "./working/resources.o"
	WINDRES = "x86_64-w64-mingw32-windres"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		LIBS += -D USE_GUI `/usr/i686-w64-mingw32/bin/wx-config --cxxflags --libs --static`
	endif
	STRIP = "x86_64-w64-mingw32-strip"
endif

ifeq ($(TARGET_PLATFORM), MACOS)
	PLATFORM = MACOS
	EXECUTABLE = "$(PROGRAM_NAME)"
	CC = "clang++"
	CCP = "clang++"
	CFLAGS += -framework IOKit -framework CoreFoundation -stdlib=libc++ -arch i386 -arch x86_64
	CFLAGS_GENERIC += -stdlib=libc++
	ARCHIVE = "$(PROGRAM_NAME) macOS.dmg"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		LIBS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
	STRIP = "strip"
endif

ifeq ($(TARGET_PLATFORM), LINUX32)
	PLATFORM = LINUX
	EXECUTABLE = "$(PROGRAM_NAME)"
	CC = "g++"
	CCP = "g++"
	CFLAGS += -pthread -static-libgcc -static-libstdc++ -m32
	CFLAGS_GENERIC += -static-libgcc -static-libstdc++
	ARCHIVE = "$(PROGRAM_NAME) Linux.zip"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		LIBS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
	STRIP = "strip"
endif

ifeq ($(TARGET_PLATFORM), LINUX64)
	PLATFORM = LINUX
	EXECUTABLE = "$(PROGRAM_NAME)"
	CC = "g++"
	CCP = "g++"
	CFLAGS += -pthread -static-libgcc -static-libstdc++ -m64
	CFLAGS_GENERIC += -static-libgcc -static-libstdc++
	ARCHIVE = "$(PROGRAM_NAME) Linux.zip"
	ifeq ($(USE_GUI), TRUE)
		SRCS += gui.cpp
		LIBS += -D USE_GUI `wx-config --prefix=/usr --cxxflags --libs --static`
	endif
	STRIP = "strip"
endif

$(EXECUTABLE): externalResources $(OBJ_FILES)
	$(CC) -o $(EXECUTABLE) $(OBJ_FILES) $(EXTRA_FILES) $(LIBS) -D $(PLATFORM) $(CFLAGS)
	$(STRIP) $(EXECUTABLE)

working/%.o: %.cpp
	$(CC) $(LIBS) -D $(PLATFORM) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLE) $(ARCHIVE) "./working"

run:
	"./$(subst $\",,$(EXECUTABLE))"

externalResources:
	mkdir -p "./working"
	rm -f "./working/resources.h"
	convert "./resources/files/images/icon.png" -resize 256x256 "./working/icon.png" || true
	$(CCP) $(CFLAGS_GENERIC) "./bin2h.cpp" -o "./working/bin2h"
	cd "./working" && find "../resources/files" -type f -exec "./bin2h" "{}" \;
	cd "./working" && "./bin2h" "../../iMe $(IME_ROM_VERSION_STRING).hex"
	cd "./working" && "./bin2h" "../resources/Linux/90-micro-3d-local.rules"
	cd "./working" && "./bin2h" "../resources/Linux/91-micro-3d-heatbed-local.rules"
	cd "./working" && "./bin2h" "../resources/Linux/92-m3d-pro-local.rules"
	cd "./working" && "./bin2h" "../resources/Linux/93-micro+-local.rules"
	cd "./working" && "./bin2h" "../resources/Windows/M3D_v2.cat"
	cd "./working" && "./bin2h" "../resources/Windows/M3D_v2.inf"
	
	if [[ "$(PLATFORM)" == "WINDOWS" ]]; then \
		convert "./resources/files/images/icon.png" -define icon:auto-resize="256,128,64,48,32,24,16" "./working/icon.ico"; \
		$(WINDRES) "./resources/Windows/resources.rc" -o "./working/resources.o"; \
	fi

package: $(EXECUTABLE)
	if [[ "$(PLATFORM)" == "MACOS" ]]; then \
		rm -rf "./working/template.app" "./working/$(PROGRAM_NAME).app" "./working/applet.iconset" "./working/applet.icns" "./working/icon.rsrc" "./working/volume.iconset" "./working/volume.icns" "./working/temp.dmg" $(ARCHIVE); \
		cp -r "./resources/macOS/template.app" "./working"; \
		mv $(EXECUTABLE) "./working/template.app/Contents/MacOS"; \
		mkdir "./working/applet.iconset"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 16 16 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_16x16.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 32 32 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_16x16@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 32 32 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_32x32.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 64 64 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_32x32@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 128 128 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_128x128.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 256 256 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_128x128@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 256 256 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_256x256.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 512 512 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_256x256@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 512 512 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_512x512.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 1024 1024 "./resources/files/images/icon.png" --out "working/applet.iconset/icon_512x512@2x.png"; \
		iconutil --convert icns "./working/applet.iconset"; \
		rm -r "./working/applet.iconset"; \
		cp "./working/applet.icns" "./working/template.app/Contents/resources"; \
		sed -i '' -e 's/Template/$(PROGRAM_NAME)/g' "./working/template.app/Contents/Info.plist"; \
		mv "./working/template.app" "./working/$(PROGRAM_NAME).app"; \
		hdiutil create -volname "$(PROGRAM_NAME)" -srcfolder "./working/$(PROGRAM_NAME).app" -format UDRW -o "./working/temp.dmg"; \
		rm -rf "./working/$(PROGRAM_NAME).app"; \
		hdiutil attach "./working/temp.dmg"; \
		sleep 2; \
		ln -s "/Applications" "/Volumes/$(PROGRAM_NAME)/ "; \
		mkdir "./working/volume.iconset"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 16 16 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_16x16.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 32 32 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_16x16@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 32 32 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_32x32.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 64 64 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_32x32@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 128 128 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_128x128.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 256 256 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_128x128@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 256 256 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_256x256.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 512 512 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_256x256@2x.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 512 512 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_512x512.png"; \
		sips -s dpiWidth 72 -s dpiHeight 72 -z 1024 1024 "./resources/macOS/volume.png" --out "working/volume.iconset/icon_512x512@2x.png"; \
		iconutil --convert icns "./working/volume.iconset"; \
		rm -r "./working/volume.iconset"; \
		sips -i "./working/volume.icns"; \
		cp "./working/volume.icns" "/Volumes/$(PROGRAM_NAME)/.VolumeIcon.icns"; \
		rm "./working/volume.icns"; \
		SetFile -a C "/Volumes/$(PROGRAM_NAME)"; \
		mkdir "/Volumes/$(PROGRAM_NAME)/.background"; \
		sips -s dpiWidth 72 -s dpiHeight 72 "./resources/macOS/background.png" --out "/Volumes/$(PROGRAM_NAME)/.background/background.png"; \
		osascript \
			-e 'tell application "Finder"' \
				-e 'tell disk "$(PROGRAM_NAME)"' \
					-e 'open' \
					-e 'delay 2' \
					-e 'set current view of container window to icon view' \
					-e 'set toolbar visible of container window to false' \
					-e 'set statusbar visible of container window to false' \
					-e 'set the bounds of container window to {240, 180, 720, 720}' \
					-e 'set viewOptions to the icon view options of container window' \
					-e 'set arrangement of viewOptions to not arranged' \
					-e 'set icon size of viewOptions to 128' \
					-e 'delay 2' \
					-e 'set background picture of viewOptions to file ".background:background.png"' \
					-e 'delay 2' \
					-e 'set position of item "$(PROGRAM_NAME).app" of container window to {240, 122}' \
					-e 'delay 2' \
					-e 'set position of item " " of container window to {240, 387}' \
					-e 'delay 2' \
					-e 'close' \
					-e 'open' \
					-e 'update without registering applications' \
					-e 'delay 2' \
					-e 'eject' \
					-e 'delay 2' \
				-e 'end tell' \
			-e 'end tell' \
		; \
		sync; \
		hdiutil convert -format UDZO -imagekey zlib-level=9 -o "./working/$(subst $\",,$(ARCHIVE))" "./working/temp.dmg"; \
		rm "./working/temp.dmg"; \
		sips -i "./working/applet.icns"; \
		DeRez -only icns "./working/applet.icns" > "./working/icon.rsrc"; \
		SetFile -a C "./working/$(subst $\",,$(ARCHIVE))"; \
		SetFile -a E "./working/$(subst $\",,$(ARCHIVE))"; \
		Rez -append "./working/icon.rsrc" -o "./working/$(subst $\",,$(ARCHIVE))"; \
		rm "./working/applet.icns" "./working/icon.rsrc"; \
		mv "./working/$(subst $\",,$(ARCHIVE))" $(ARCHIVE); \
	else \
		rm -f $(ARCHIVE); \
		zip -9 -r $(ARCHIVE) $(EXECUTABLE); \
	fi
	rm -rf $(EXECUTABLE)
