#!/bin/sh -e
T=/tmp/make-initrd.$$
C=/tmp/make-initrd-cramfs.$$

DIRS="dev proc images"
DEVS="loop0 console std hda"

if [ -e $T -o -e C ]
then
	echo wail! temporary foo exists
	exit 1
fi

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
rm $T/dev/MAKEDEV

echo making cramfs
mkcramfs $T $C

echo new initrd is at $C
