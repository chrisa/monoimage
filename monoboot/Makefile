MB_CFLAGS += -g -Wall -pedantic -std=c99
MB_LIBS   += -lconfuse -lreadline

CFLAGS += $(MB_CFLAGS)
LDLIBS += $(MB_LIBS)

all:		monoboot

monoboot:	monoboot.c monoboot.h
	gcc -c $(CFLAGS) monoboot.c
	gcc -o monoboot $(LDLIBS) monoboot.o

clean: 
	rm -f monoboot *.o

.PHONY: clean
