#!/bin/sh -e
T=tmp
C=../initrd.img

DIRS="dev proc images"
DEVS="loop console std hda"

echo removing old initrd tree
rm -Rf $T
mkdir $T

for D in $DIRS
do
	echo making $D
	mkdir $T/$D
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
mkcramfs $T $C

echo new initrd is at $C
