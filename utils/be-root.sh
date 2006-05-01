#!/bin/sh -e

SCRIPT=$1
FAKEROOT=/usr/bin/fakeroot
if [ ! -x $FAKEROOT ]; then
    USER=`whoami`
    if [ "$USER" != "root" ]; then
	echo "you must be root, or have $FAKEROOT"
	exit 1;
    fi
    USEFAKEROOT=0
else
    USEFAKEROOT=1
fi

if [ $USEFAKEROOT = 1 ]; then
    $FAKEROOT /bin/sh $SCRIPT
else 
    /bin/sh $SCRIPT
fi
