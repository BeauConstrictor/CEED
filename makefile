ceed: *.c *.h 
	gcc ceed.c -o ceed

run: ceed
	./ceed
