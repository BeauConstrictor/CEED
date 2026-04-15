ceed: src/*
	gcc src/ceed.c -o ceed

run: ceed
	./ceed
