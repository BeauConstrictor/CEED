CFGCMDS := $(wildcard src/cfglib/*)
CFILES  := $(wildcard src/*.[ch])

CCOMP := gcc
CCARGS := -Wall -Wextra -Werror -Wno-unused-parameter

all: ceed build/cfglib

build/:
	mkdir -p build/

csrpc:
	$(MAKE) -C lib/csrpc/

build/cfglib: $(CFGCMDS) csrpc
	mkdir -p build/cfglib
	cp src/cfglib/* build/cfglib
	cp lib/csrpc/build/send-cmd build/cfglib
	chmod +x build/cfglib/*

ceed: $(CFILES) build/ csrpc
	$(CCOMP) $(CCARGS) src/*.c -o build/ceed -L./lib/csrpc/build/ \
	  -lcsrpc -I./lib/csrpc/include

.PHONY: all run

run: all
	build/ceed --repl
