CC=gcc
CFLAGS=-std=c99 -O0

all: dcpu16

dcpu16: dcpu16.c
	mkdir -p bin
	$(CC) $(CFLAGS) dcpu16.c -o bin/dcpu16

clean:
	rm bin/dcpu16
