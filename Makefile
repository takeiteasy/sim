ifeq ($(OS),Windows_NT)
	PROG_EXT=.exe
	LIB_EXT=dll
	CFLAGS=-O2 -DSOKOL_D3D11 -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
	ARCH=win32
	SHDC_FLAGS=hlsl5
else
	UNAME:=$(shell uname -s)
	PROG_EXT=
	ifeq ($(UNAME),Darwin)
		LIB_EXT=dylib
		CFLAGS=-x objective-c -DSOKOL_METAL -fobjc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox
		ARCH:=$(shell uname -m)
		SHDC_FLAGS=metal_macos
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
	else ifeq ($(UNAME),Linux)
		LIB_EXT=so
		CFLAGS=-DSOKOL_GLCORE33 -pthread -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor
		ARCH=linux
		SHDC_FLAGS=glsl330
	else
		$(error OS not supported by this Makefile)
	endif
endif

NAME=sim
INCLUDE=-Ideps -Isrc -Ibuild
ARCH_PATH=deps/sokol-tools-bin/bin/$(ARCH)
SHDC_PATH=$(ARCH_PATH)/sokol-shdc$(PROG_EXT)

default: test

shader:
	$(SHDC_PATH) -i src/sim.glsl -o src/sim.glsl.h -l $(SHDC_FLAGS)

library: shader
	$(CC) $(INCLUDE) -shared -fpic $(CFLAGS) src/sim.c -o build/lib$(NAME).$(LIB_EXT)

test: library
	$(CC) $(INCLUDE) $(EXTRA_CFLAGS) $(CFLAGS) src/*.c -o build/$(NAME)_test$(PROG_EXT)

all: shader library test

.PHONY: default all library test shaders
