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
		CFLAGS=-x objective-c++ -DSOKOL_METAL -fobjc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox
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
EXTRA_CFLAGS=-shared -fpic -std=c++11 -lstdc++
INCLUDE=-Ideps -Isrc -Ibuild

SHDC_PATH=./bin/$(ARCH)/sokol-shdc$(PROG_EXT)
SHADERS=$(wildcard src/*.glsl)
SHADER_OUTS=$(patsubst %,%.h,$(SHADERS))

all: default

.SECONDEXPANSION:
SHADER=$(patsubst %.h,%,$@)
SHADER_OUT=$@
%.glsl.h: $(SHADERS)
	$(SHDC_PATH) -i $(SHADER) -o $(SHADER_OUT) -l glsl330:metal_macos:hlsl5:wgpu -b

shaders: $(SHADER_OUTS)
	rm src/*.dia		|| yes
	rm src/*.air		|| yes
	rm src/*.metal		|| yes
	rm src/*.metallib	|| yes

default: library

library: shaders
	$(CC) $(INCLUDE) $(EXTRA_CFLAGS) $(CFLAGS) src/sim.cpp -o build/$(NAME).$(LIB_EXT)

.PHONY: all library shaders
