#!/bin/sh

# script to partition hda into three and install kernel and rootfs
# on hda1. it's intended to be run on a netbooted soekris.
#
# this script lives in a /tftpboot-style place
# it assumes you've just netbooted into a buildroot'd initrd
#
# reqs:
#   kernel is available at ${SERVER}:/tftpboot/${KERNEL}
#   buildroot tar.gz target is available at ${SERVER}/tftpboot/${ROOTFS_TGZ}
#   your netboot initrd has tftp, gnutar, grub and mke2fs packages 
#   that your kernel and rootfs are correctly named below

# launch it by running on the newly booted soekris something like:
#   udhcpc eth0
#   SERVER=192.168.237.8 && export SERVER && cd /var/tmp
#   tftp -g -l setup-cf.sh -r setup-cf.sh ${SERVER} && sh ./setup-cf.sh

# TODO:
#   don't bin errors, deal with them
#   reduce the amount of rampant heredoc'ing

KERNEL=bzImage-kexec
ROOTFS_TGZ=buildroot.tar.gz

# functions
_partition()
{
    echo partitioning flash

    # shonky
    fdisk /dev/hda > /dev/null 2>&1 <<EOP
d
1
d
2
d
3
d
4

n
p
1

+32M
n
p
2

+32M
n
p
3


a
1
w
q
EOP

#    fdisk /dev/hda 2> /dev/null <<EOP
#p
#q
#EOP

}

_usage()
{
    echo please set SERVER to your tftp server
    exit 1
}

_format()
{
    echo making new filesystems
    mke2fs /dev/hda1 >/dev/null 2>&1
    mke2fs /dev/hda2 >/dev/null 2>&1
    mke2fs /dev/hda3 >/dev/null 2>&1
}

_mount_mnt()
{
    echo mounting ext2 $1 at /mnt
    mount -t ext2 $1 /mnt
}

_umount_mnt()
{
    echo unmounting /mnt
    sync
    umount /mnt
}

_copy_kernel()
{
    echo copying kernel from ${SERVER}
    tftp -g -r $1 -l /mnt/boot/$1 ${SERVER}
}

_copy_fs()
{
    echo copying root fs from ${SERVER}:$1
    tftp -g -r $1 -l /var/tmp/$1 ${SERVER}
    cd /mnt
    busybox tar zxf /var/tmp/$1
    cd /
}

_copy_grub()
{
    echo installing grub
    mkdir -p /mnt/boot/grub
    cp /mnt/usr/share/grub/i386-pc/* /mnt/boot/grub
    # more heredoc joy
    cat > /mnt/boot/grub/menu.lst <<EOM
serial --unit=0 --speed=19200
terminal serial

default 0
timeout 1

title=kexec
root (hd0,0)
kernel /boot/${KERNEL} console=ttyS0,19200n8 rw root=/dev/hda1 ide=nodma
EOM

    # grub-install should be able to do this cleanly
    # but i can't make it work
    grub <<EOG >/dev/null 2>&1
root (hd0,0)
setup (hd0)
EOG
}

_check_kernel()
{
    if [ -r /mnt/boot/$1 ]
    then
       echo woo kernel copied ok
    else
       echo arg kernel didn\'t copy 
    fi
}

# main

if [ -z ${SERVER} ]
then
    _usage
fi

_umount_mnt
_partition
_format

_mount_mnt "/dev/hda1"
_copy_fs ${ROOTFS_TGZ}
_copy_grub
_copy_kernel ${KERNEL}
_umount_mnt

_mount_mnt "/dev/hda1"
_check_kernel ${KERNEL}
_umount_mnt

echo
echo all done.
echo

exit 0
