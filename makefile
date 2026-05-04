all: ceed

build/:
	mkdir -p build

ceed: src/* build/
	gcc -Wall -Wextra src/*.c -o build/ceed

.PHONY: all run

run: ceed
	build/ceed src/main.c
