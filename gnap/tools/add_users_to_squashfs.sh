#!/bin/bash

set -e

# This wee script takes a core file, grabs passwd shadow and group files
# and appends our own user info to them, and stores the results in OUT_DIR
# It should be run once against each new gnap core file, to ensure the users
# exist for any new software installed
# The files produced should be placed in an overlay directory ready for inclusion

#CORE_FILE="/home/dunc/work/suppose-gnap/gnap-2.0rc1-20060410.tar"
#EXTRA_PASSWD_FILE="/home/dunc/work/suppose-gnap/conf/passwd.extras"
#EXTRA_GROUP_FILE="/home/dunc/work/suppose-gnap/conf/group.extras"
#EXTRA_SHADOW_FILE="/home/dunc/work/suppose-gnap/conf/shadow.extras"
#OUT_DIR="/home/dunc/work/suppose-gnap/out"

TMP_DIR="/tmp/gnap_user_merge_$$"
SQUASH_MOUNTPOINT="${TMP_DIR}/squash"

usage() {
        echo 'Options:'
        echo '    -h                            This Help'
        echo '    -c core_file                  Which gnap core file to use'
        echo '    -p extra_password_file        Passwd file which will be added to the core passwd file'
        echo '    -g extra_group_file           Group file which will be added to the core group file'
        echo '    -s extra_shadow_file          Shadow file which will be added to the core shadow file'
        echo '    -o output_dir                 Where to put the resultant files'
}

test_options() {
        if [[ ! -f ${CORE_FILE} ]]; then
                echo "Cannot find core file ${CORE_FILE}"
                exit 1
        fi
        if [[ ! -f ${EXTRA_PASSWD_FILE} ]]; then
                echo "Cannot find extra passwd file ${EXTRA_PASSWD_FILE}"
                exit 1
        fi
        if [[ ! -f ${EXTRA_GROUP_FILE} ]]; then
                echo "Cannot find extra group file ${EXTRA_GROUP_FILE}"
                exit 1
        fi
        if [[ ! -f ${EXTRA_SHADOW_FILE} ]]; then
                echo "Cannot find extra shadow file ${EXTRA_SHADOW_FILE}"
                exit 1
        fi
        if [[ ! -d ${OUT_DIR} ]]; then
                echo "${OUT_DIR} is not a directory"
                exit 1
        fi
}

while getopts ':c:hp:g:s:o:' options; do
	case ${options} in
		h ) usage
			exit 0;;
		c ) CORE_FILE="${OPTARG}";;
		p ) EXTRA_PASSWD_FILE="${OPTARG}";;
		g ) EXTRA_GROUP_FILE="${OPTARG}";;
		s ) EXTRA_SHADOW_FILE="${OPTARG}";;
		o ) OUT_DIR="${OPTARG}";;
		* ) echo 'Unknown options specified'
			usage
			exit 0;;
	esac

done

test_options

mkdir ${TMP_DIR}
mkdir ${SQUASH_MOUNTPOINT}

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

