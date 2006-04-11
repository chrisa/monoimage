#!/bin/bash

set -e

# This wee script takes a core file, grabs passwd shadow and group files
# and appends our own user info to them, and stores the results in OUT_DIR
# It should be run once against each new gnap core file, to ensure the users
# exist for any new software installed
# The files produced should be placed in an overlay directory ready for inclusion

CORE_FILE="/home/dunc/work/suppose-gnap/gnap-2.0rc1-20060410.tar"
SQUASH_MOUNTPOINT="/mnt/gnap/squash"
EXTRA_PASSWD_FILE="/home/dunc/work/suppose-gnap/conf/passwd.extras"
EXTRA_GROUP_FILE="/home/dunc/work/suppose-gnap/conf/group.extras"
EXTRA_SHADOW_FILE="/home/dunc/work/suppose-gnap/conf/shadow.extras"
OUT_DIR="/home/dunc/work/suppose-gnap/out"

TMP_DIR="/tmp/gnap_user_$$"

mkdir ${TMP_DIR}
tar -xf ${CORE_FILE} -C ${TMP_DIR}

#now mount the squashfs file
mount -o loop ${TMP_DIR}/image.squashfs ${SQUASH_MOUNTPOINT}

#append foo to files
cat ${SQUASH_MOUNTPOINT}/etc/passwd ${EXTRA_PASSWD_FILE} > ${OUT_DIR}/passwd
cat ${SQUASH_MOUNTPOINT}/etc/group ${EXTRA_GROUP_FILE} > ${OUT_DIR}/group
cat ${SQUASH_MOUNTPOINT}/etc/shadow ${EXTRA_SHADOW_FILE} > ${OUT_DIR}/shadow

#umount squashfs
umount ${SQUASH_MOUNTPOINT}

rm -fr ${TMP_DIR}

