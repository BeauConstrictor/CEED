cedit: *.c *.h 
	gcc main.c -o cedit

run: cedit
	./cedit
