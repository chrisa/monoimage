
linuxrc: linuxrc.c ../include/monoimage.h loop.h loop_dev.h
	$(CC) -Wall -pedantic -std=c99 -I../include -static -o linuxrc linuxrc.c

clean:
	rm -f linuxrc

all: linuxrc

.PHONY: clean
