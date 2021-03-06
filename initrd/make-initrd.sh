#!/bin/sh -e

T=tmp
C=initrd.img

DIRS="dev proc images lib bin"
DEVS="loop console std hda hdc"
LIBS="libncurses.so.5 libdl.so.2 libc.so.6 ld-linux.so.2 libblkid.so.1 libuuid.so.1"
BINS="sh mount cat"

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

echo removing old initrd tree
rm -Rf $T
mkdir $T

for D in $DIRS
do
	echo making $D
	mkdir $T/$D
done

for L in $LIBS
do
	echo copying library $L
	cp /lib/$L $T/lib/$L
done

for B in $BINS
do
	echo copying binary $B
	cp /bin/$B $T/bin/$B
done

echo copying in linuxrc
cp ../linuxrc/linuxrc $T/linuxrc

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
$MKCRAMFS $T $C

echo new initrd is at $C
