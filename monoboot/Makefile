# $Id$ 

MB_CFLAGS += -g -Wall -pedantic -std=c99
MB_LIBS   += -lconfuse -lncurses -lreadline

CFLAGS += $(MB_CFLAGS)
LDLIBS += $(MB_LIBS)

AR     ?= /usr/bin/ar
RANLIB ?= /usr/bin/ranlib
CC     ?= /usr/bin/gcc

all: monoboot mbsh monoinit

libmonoboot.a: monoboot.h monoboot_cmds.c \
          monoboot_cli.c monoboot_exec.c monoboot_util.c
	$(CC) -c $(CFLAGS) monoboot_cmds.c
	$(CC) -c $(CFLAGS) monoboot_cli.c
	$(CC) -c $(CFLAGS) monoboot_exec.c
	$(CC) -c $(CFLAGS) monoboot_util.c
	$(AR) cru libmonoboot.a monoboot_cmds.o monoboot_cli.o \
				monoboot_exec.o monoboot_util.o
	$(RANLIB) libmonoboot.a

monoboot: libmonoboot.a monoboot.c
	$(CC) -c $(CFLAGS) monoboot.c
	$(CC) -o monoboot $(LDLIBS) monoboot.o libmonoboot.a

monoinit: libmonoboot.a monoinit.c
	$(CC) -c $(CFLAGS) monoinit.c
	$(CC) -o monoinit $(LDLIBS) monoinit.o libmonoboot.a 

mbsh: monoboot
	ln -f monoboot mbsh

clean: 
	rm -f monoboot mbsh monoinit *.o *.a

install:
	cp monoboot $(DESTDIR)/bin
	ln -f $(DESTDIR)/bin/monoboot $(DESTDIR)/bin/mbsh

.PHONY: clean
