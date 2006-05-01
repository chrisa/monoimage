
linuxrc: linuxrc.c ../include/monoimage.h loop.h loop_dev.h
	$(CC) -DDEBUG -Wall -pedantic -std=c99 -I../include -static -o linuxrc linuxrc.c -lext2fs

clean:
	rm -f linuxrc

all: linuxrc

.PHONY: clean
