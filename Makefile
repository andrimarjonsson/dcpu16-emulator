all:
	mkdir -p bin
	gcc dcpu16.c -std=c99 -O3 -o bin/dcpu16


clean:
	rm bin/dcpu16
