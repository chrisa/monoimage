#!/bin/sh -e
T=tmp
F=../rootfs.img
M=mnt

DIRS="sbin lib initrd dev"
DEVS="std console"
LIBS="libncurses.so.5 libdl.so.2 libc.so.6 ld-linux.so.2"

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

echo making loopback file
dd if=/dev/zero of=$F bs=1024k count=4

echo mounting it
losetup /dev/loop/0 $F
mkfs -t ext2 /dev/loop/0 4096
mount /dev/loop/0 $M

echo copying stuff in
cp -pR $T/* $M/

echo unmounting
umount $M

echo unlooping
losetup -d /dev/loop/0

echo new rootfs is at $F
