# $Id$ 

MB_CFLAGS += -g -Wall -pedantic -std=c99
MB_LIBS   += -lconfuse -lreadline

CFLAGS += $(MB_CFLAGS)
LDLIBS += $(MB_LIBS)

all: monoboot mbsh

monoboot: monoboot.c monoboot.h monoboot_cmds.c \
          monoboot_cli.c monoboot_exec.c 
	gcc -c $(CFLAGS) monoboot.c
	gcc -c $(CFLAGS) monoboot_cmds.c
	gcc -c $(CFLAGS) monoboot_cli.c
	gcc -c $(CFLAGS) monoboot_exec.c
	gcc -c $(CFLAGS) monoboot_util.c
	gcc -o monoboot $(LDLIBS) monoboot.o monoboot_cmds.o monoboot_cli.o \
                                  monoboot_exec.o monoboot_util.o

mbsh: monoboot
	ln -f monoboot mbsh

clean: 
	rm -f monoboot mbsh *.o

.PHONY: clean
