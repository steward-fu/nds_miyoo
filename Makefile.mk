SDL2_CFG+= --disable-joystick-virtual
SDL2_CFG+= --disable-jack
SDL2_CFG+= --disable-power
SDL2_CFG+= --disable-sensor
SDL2_CFG+= --disable-ime
SDL2_CFG+= --disable-dbus
SDL2_CFG+= --disable-fcitx
SDL2_CFG+= --disable-hidapi
SDL2_CFG+= --disable-libudev
SDL2_CFG+= --disable-video-x11
SDL2_CFG+= --disable-video-kmsdrm
SDL2_CFG+= --disable-video-vulkan
SDL2_CFG+= --disable-video-wayland
SDL2_CFG+= --disable-video-dummy
SDL2_CFG+= --disable-video-opengl
SDL2_CFG+= --disable-video-opengles
SDL2_CFG+= --disable-video-opengles2
SDL2_CFG+= --disable-sndio
SDL2_CFG+= --disable-diskaudio
SDL2_CFG+= --disable-pulseaudio
SDL2_CFG+= --disable-dummyaudio
SDL2_CFG+= --disable-oss
SDL2_CFG+= --disable-alsa
SDL2_CFG+= --enable-loadso
SDL2_CFG+= --enable-joystick

$(shell cd sdl2 && rm -rf libEGL.so libGLESv2.so)
REL_VER  = $(shell printf `git rev-parse HEAD | cut -c 1-8`_`date +%Y%m%d%H%M%S`)

ifeq ($(MOD),mini)
    CFLAGS  += -O3
    CFLAGS  += -DMINI
    CFLAGS  += -mcpu=cortex-a7
    CFLAGS  += -mfpu=neon-vfpv4
    CFLAGS  += -I../alsa
    CFLAGS  += -I../detour
    CFLAGS  += -I../common
    CFLAGS  += -I../include/mini
    LDFLAGS += -L../library/mini
    LDFLAGS += -lmi_ao
    LDFLAGS += -lmi_sys
    LDFLAGS += -lmi_gfx
    LDFLAGS += -lmi_common
    LDFLAGS += -Wl,--no-undefined
endif

ifeq ($(MOD),a30)
    CFLAGS  += -O3
    CFLAGS  += -DA30
    CFLAGS  += -I../alsa
    CFLAGS  += -I../detour
    CFLAGS  += -I../common
    CFLAGS  += -mcpu=cortex-a7
    CFLAGS  += -mfpu=neon-vfpv4
    LDFLAGS += -Wl,--no-undefined
endif

ifeq ($(MOD),ut)
    export ASAN_OPTIONS=detect_odr_violation=0
    CFLAGS  += -DUT
    CFLAGS  += -I../alsa
    CFLAGS  += -I../detour
    CFLAGS  += -I../common
    CFLAGS  += -I../ut/src
    CFLAGS  += -I../ut/extras/memory/src
    CFLAGS  += -I../ut/extras/fixture/src
    CFLAGS  += -fno-omit-frame-pointer
    CFLAGS  += -fsanitize=address,leak,undefined
endif

export CC=${CROSS}gcc
export AR=${CROSS}ar
export AS=${CROSS}as
export LD=${CROSS}ld
export CXX=${CROSS}g++
export MOREFLAGS=${CFLAGS} ${LDFALGS}

.PHONY: all
all: cfg
	MOD=$(MOD) make -C common
	cp common/libcommon.so drastic/libs/
	MOD=$(MOD) make -C detour
	cp detour/libdetour.so drastic/libs/
	MOD=$(MOD) make -C alsa
	cp alsa/libasound.so.2 drastic/libs/
	MOD=$(MOD) make -C sdl2 -j4
	cp sdl2/build/.libs/libSDL2-2.0.so.0 drastic/libs/
	MOD=$(MOD) make -C ut $(MOD)

.PHONY: cfg
cfg:
ifeq (,$(wildcard sdl2/Makefile))
	cp -a ChangeLog.txt drastic/
	cp -a assets/$(MOD)/* drastic/
	cd sdl2 && ./autogen.sh && MOD=$(MOD) ./configure $(SDL2_CFG) --host=$(HOST)
endif

.PHONY: rel
rel:
	zip -r drastic_$(MOD)_$(REL_VER).zip drastic

.PHONY: clean
clean:
	rm -rf ut/ut
	rm -rf drastic/libs2
	rm -rf drastic/cpuclock
	rm -rf drastic/launch.sh
	rm -rf drastic/config.json
	rm -rf drastic/show_hotkeys
	rm -rf drastic/ChangeLog.txt
	rm -rf drastic/miyoo/settings.pb
	rm -rf drastic/system
	rm -rf drastic/libs/libdetour.so
	rm -rf drastic/libs/libcommon.so
	rm -rf drastic/libs/libasound.so.2
	rm -rf drastic/libs/libpcre2-8.so.0
	rm -rf drastic/libs/libSDL2-2.0.so.0
	rm -rf drastic/libs/libfreetype.so.6
	rm -rf drastic/libs/libglib-2.0.so.0
	rm -rf drastic/libs/libharfbuzz.so.0
	make -C ut clean
	make -C alsa clean
	make -C detour clean
	make -C common clean
	make -C sdl2 distclean > /dev/null 2>&1 || true
	sed -i 's/screen_orientation.*/screen_orientation = 0/g' drastic/config/drastic.cfg
	cd drastic && mkdir -p system backup scripts slot2 unzip_cache cheats input_record profiles savestates
