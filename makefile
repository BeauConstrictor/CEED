CFGCMDS := $(wildcard src/cfglib/*)
CFILES  := $(wildcard src/*.[ch])

CCOMP := gcc
CCARGS := -Wall -Wextra -Werror -Wno-unused-parameter

all: build/ceed build/cfglib

csrpc:
	$(MAKE) -C lib/csrpc/

build/:
	mkdir -p build/

build/cfglib: $(CFGCMDS) build/ csrpc
	mkdir -p build/cfglib
	cp src/cfglib/* build/cfglib
	cp lib/csrpc/build/send-cmd build/cfglib
	chmod +x build/cfglib/*

build/ceed: $(CFILES) build/ csrpc
	$(CCOMP) $(CCARGS) src/*.c -o build/ceed -L./lib/csrpc/build/ \
	  -lcsrpc -I./lib/csrpc/include

.PHONY: all run

run: export CEED_INSTALL=./build
run: all
	build/ceed src/main.c

install:
	mkdir -p $(HOME)/.local/share
	mkdir -p $(HOME)/.local/bin
	cp -r build/ $(HOME)/.local/share/ceed/
	cp build/ceed $(HOME)/.local/bin/ceed
