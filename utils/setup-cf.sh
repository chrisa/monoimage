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
#  SERVER=192.168.237.8 && export SERVER && cd /var/tmp
#  tftp -g -l setup-cf.sh -r setup-cf.sh ${SERVER} && sh ./setup-cf.sh ${SERVER}

# TODO:
#   don't bin errors, deal with them
#   reduce the amount of rampant heredoc'ing

KERNEL=bzImage-kexec
ROOTFS_TGZ=root_fs_i386.tar.gz
MONOIMAGE=monoimage

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
    echo setup-cf.sh \<server\>
    exit 1
}

_format()
{
    echo making new filesystems
    mke2fs -j /dev/hda1 >/dev/null 2>&1
    mke2fs -j /dev/hda2 >/dev/null 2>&1
    mke2fs -j /dev/hda3 >/dev/null 2>&1
}

_mount_mnt()
{
    echo mounting ext3 $1 at /mnt
    mount -t ext3 $1 /mnt 
}

_umount_mnt()
{
    echo unmounting /mnt
    sync
    umount /mnt
}

_clear_mounts()
{
    echo clearing out stage1 mounts
    umount /mnt
    umount /config
    umount /images
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

_copy_monoimage()
{
    echo copying monoimage from ${MONOIMAGE}
    tftp -g -r $1 -l /mnt/$1 ${SERVER}
    mv /mnt/$1 /mnt/default.img
}

_unpack_skel()
{
    echo unpacking $1 onto /mnt
    cd /mnt
    busybox tar zxf $1
    chown -R root:root *
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

_install_mbconf()
{
    echo installing default mb.conf
    cp /usr/share/funknet/mb.conf-sample /mnt/mb.conf
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

if [ -z $1 ]
then
    _usage
fi
SERVER=$1

_clear_mounts
_partition
_format

_mount_mnt "/dev/hda1"
_copy_fs ${ROOTFS_TGZ}
_copy_grub
_copy_kernel ${KERNEL}
_umount_mnt

_mount_mnt "/dev/hda2"
_unpack_skel "/usr/share/funknet/config-skel.tar.gz"
_install_mbconf 
_umount_mnt

_mount_mnt "/dev/hda3"
_copy_monoimage ${MONOIMAGE}
_umount_mnt

_mount_mnt "/dev/hda1"
_check_kernel ${KERNEL}
_umount_mnt

echo
echo all done.
echo

exit 0

