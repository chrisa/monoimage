MB_CFLAGS +=	`confuse-config --cflags`
MB_LIBS +=	`confuse-config --libs`

CFLAGS += $(MB_CFLAGS)
LDLIBS += $(MB_LIBS)

all:		monoboot

monoboot:	monoboot.c monoboot.h
