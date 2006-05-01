#!/bin/sh -e
T=tmp
F=../rootfs.img
M=mnt

DIRS="sbin lib initrd dev"
DEVS="std console"
LIBS="libncurses.so.5 libdl.so.2 libc.so.6 ld-linux.so.2"

if [ -x /usr/sbin/mkcramfs ]
then
	MKCRAMFS="/usr/sbin/mkcramfs"
elif [ -x /sbin/mkcramfs ]
then
	MKCRAMFS="/sbin/mkcramfs"
else
	echo no mkcramfs found
	exit 1
fi

rm -Rf $T
rm -Rf $M
mkdir $T
mkdir $M

for D in $DIRS
do
	echo making $D
	mkdir $T/$D
done

echo copying bash as init
cp /bin/bash $T/sbin/init

for L in $LIBS
do
	echo copying library $L
	cp /lib/$L $T/lib/$L
done

echo making devices
cp /dev/MAKEDEV $T/dev/MAKEDEV
cd $T/dev
for D in $DEVS
do
	echo makedev $D
	./MAKEDEV $D
done
cd ../..
rm $T/dev/MAKEDEV

echo making cramfs
$MKCRAMFS $T $F

echo new rootfs is at $F
