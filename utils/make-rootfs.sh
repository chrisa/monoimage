#!/bin/sh -e
T=/tmp/make-rootfs.$$
F=/tmp/make-rootfs-image.$$
M=/tmp/make-rootfs-mount.$$

DIRS="sbin lib initrd dev"
DEVS="std console"
LIBS="libncurses.so.5 libdl.so.2 libc.so.6 ld-linux.so.2"

if [ -e $T -o -e F -o -e $M ]
then
	echo wail! temporary foo exists
	exit 1
fi

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
rm $T/dev/MAKEDEV

echo making loopback file
dd if=/dev/zero of=$F bs=1024k count=4

echo mounting it
losetup /dev/loop0 $F
mkfs -t ext2 /dev/loop0 4096
mount /dev/loop0 $M

echo copying stuff in
cp -pR $T/* $M/

echo unmounting
umount $M

echo unlooping
losetup -d /dev/loop0

echo new rootfs is at $F
