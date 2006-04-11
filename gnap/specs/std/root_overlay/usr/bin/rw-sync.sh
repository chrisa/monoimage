#!/bin/bash

#-----------------------------------------------------------------------
# Script to save files to a rw partition or else.
# by José Alberto Suárez López <bass@gentoo.org>
# 05/07/05 16:01:27 CEST
# 06/07/05 12:29:16 CEST
# 12/07/05 16:13:14 CEST
# 13/07/05 12:39:56 CEST
#-----------------------------------------------------------------------

##
# Global variables (sourcing new in 0.6)
##
source /etc/overlay.conf
source /etc/conf.d/overlay
RWPART="${RW_SYNC}"
#rename bak to res to force bak to load

NO_ARGS="0"
version="v 0.6"
progname=`basename $0`

##
# Options
##
while getopts "a:wvhcb" miopt ; do
    case $miopt in
	a) opta="$OPTARG" ;;
	w) optw=1 ;;
	v) optv=1 ;;
	h) opth=1 ;;
	c) RC_NOCOLOR=yes ;; 
	b) optb=1 ;;         
	*) echo " " "* Try -h for help"
	    exit 1;;
    esac
done

##
# Colorize or not colorize...
##
if [[ ${RC_NOCOLOR} == yes ]]; then
    unset GOOD WARN BAD NORMAL HILITE BRACKET BOLD
else
    GOOD=$'\e[32;01m'
    WARN=$'\e[33;01m'
    BAD=$'\e[31;01m'
    NORMAL=$'\e[0m'
    HILITE=$'\e[36;01m'
    BRACKET=$'\e[34;01m'
    BOLD=$'\e[1m'
fi

eerror () {
    echo -e " ${BAD}*${NORMAL} $*"
}

ewarn () {
    echo -e " ${WARN}*${NORMAL} $*"
}

einfo () {
    echo -e " ${GOOD}*${NORMAL} $*"
}

etab () {
    echo -e "    " "$*"
}

fverb () {
    if [ -n "$optv" ] ; then
	if [ "$1" == e ] ; then
	    eerror $2
	elif [ "$1" == w ] ; then
	    ewarn $2
	else
	    einfo $*
	fi
    fi
}

##
# Help
##
ayuda () {
    echo "${HILITE}RW-Sync${NORMAL} for ${BOLD}GNAP${NORMAL} ${BRACKET}[${NORMAL} ${GOOD}${version}${NORMAL} ${BRACKET}]${NORMAL}"
    echo -e " " "by bass@gentoo.org"
    echo
    einfo "Usage: ${progname} -a [FILE] -wvhc"
    echo
    etab "${BOLD}-a FILE${NORMAL} : Add FILE to ${CONFFILE}."
    etab "${BOLD}-w	${NORMAL} : Save conf to ${MNTDIR}/${TARFILE}."
    etab "${BOLD}-v	${NORMAL} : Verbose."
    etab "${BOLD}-b	${NORMAL} : Make backup to ${MNTDIR}/${BAKFILE}."
    etab "${BOLD}-c	${NORMAL} : No color."
    etab "${BOLD}-h	${NORMAL} : This help."
    echo
    einfo "Example:"
    einfo "${BOLD}${progname} -a /etc/passwd${NORMAL}"
    einfo "${BOLD}${progname} -w${NORMAL}"
    echo
    einfo "How to add files and dirs:"
    einfo "Add a file: ${BOLD}${progname} -a /etc/passwd${NORMAL}"
    einfo "Add a miltiply files: ${BOLD}${progname} -a /etc/init.d/*${NORMAL}"
    einfo			"${BOLD}${progname} -a /etc/passwd /etc/inittab${NORMAL}"
    einfo "Add a directory: ${BOLD}${progname} -a /etc/init.d/${NORMAL}"
    echo
    ewarn
    ewarn "The first file that you want add is ${BOLD}${CONFFILE}${NORMAL} ${GOOD};-)${NORMAL}"
    ewarn
}

##
# First tests (CONFTEST new in 0.4.1)
##
[ ! -d ${MNTDIR} ] && mkdir -p ${MNTDIR}
if [ -f ${RWPART} ] ; then
    [ ! -f ${RWPART} ] && eerror "RW-Partition ${BOLD}${RWPART}${NORMAL} doesn't exist" && exit 1
fi
if [ -f ${CONFFILE} ] ; then
    CONFTEST=$(cat ${CONFFILE} | grep ${CONFFILE})
    if [[ ${CONFTEST} != ${CONFFILE} ]] ;then 
	[ -n "$optv" ] && ewarn "The ${BOLD}config file: ${CONFFILE} will not to be saved${NORMAL} if you dont add itself to ${CONFFILE}"
    fi
fi

##
# Functions
##
fmount () {
    if [ -n "$RWPART" ] ; then
	mount ${RWPART} /mnt/gentoo
	fverb "${RWPART} mounted"
    fi
}
fumount () {
    if [ -n "$RWPART" ] ; then
	umount /mnt/gentoo
	fverb "${RWPART} unmounted"
    fi
}

##
# size check (new in 0.5)
##
fsize () {
    # fsize [p] part/file [n]
    if [ "$1" == p ] ; then
	set -- $(df -k | grep "${2}")
	psize="$4"
	# aviable space
    else
	set -- $(ls -lk "${1}")
	size="$5"
    fi
}

##
# Options
##
if [ $# -eq "$NO_ARGS" -o -n "$opth" ] ; then
    ayuda
fi

if [ -n "$opta" ] ; then
    fverb "Adding files to ${CONFFILE} ..."
    for i in $* ; do
	if [ $i != "-a" -a $i != "-v" -a $i != "-c" -a $i != "-b" ] ; then
		    echo $i >> ${CONFFILE} ;
	fi
    done
    fverb "Files added ${GOOD}:-)${NORMAL}"
fi

if [ -n "$optw" ] ; then
    [ ! -f ${CONFFILE} ] && eerror "Configuration file ${BOLD}${CONFFILE}${NORMAL} doesn't exist. Use -a" && exit 1
    fverb "Saving files to ${MNTDIR}/${TARFILE} ..."
    fmount
    
    ##
    # Backup creation (new in 0.3)
    ##
    if [ -n "$optb" ] ; then
	fverb "Doing backup file..."
	if [ -f ${MNTDIR}/${TARFILE} ] ; then
            fsize ${MNTDIR}/${TARFILE}
            fsize p ${RWPART}
            if [ "$psize" -gt "$size" ] ; then
		cp ${MNTDIR}/${TARFILE} ${MNTDIR}/${BAKFILE}
		fverb "Backup made."
            else
                fverb e "No sufficent space in ${RWPART}"
            fi
	else
	    fverb e "No file to backup."
	fi
    fi
    
    if [ -f ${MNTDIR}/${TARFILE} ] ; then
	fsize ${MNTDIR}/${TARFILE}
	OLDSIZE=$size
    fi
    
    set -- $(cat ${CONFFILE})
    fverb "Making tarball..."
    if [ -n "$optv" ] ; then
        tar cvfzp ${TMPDIR}/${TMPTARFILE} $*
    else
        tar cvfzp ${TMPDIR}/${TMPTARFILE} $* > /dev/null
    fi
    fsize ${TMPDIR}/${TMPTARFILE}
    tmptarsize="$size"
    [ -f ${MNTDIR}/${TARFILE} ] && fsize ${MNTDIR}/${TARFILE}
    tarfsize="$size"
    fsize p ${RWPART}
    RWPARTSIZE=$(($psize+$tarfsize))
    if [ "$RWPARTSIZE" -gt "$tmptarsize" ] ; then
	cp ${TMPDIR}/${TMPTARFILE} ${MNTDIR}/${TARFILE}
    else
        fverb e "no sufficent space in ${RWPART}"
    fi
    fverb "Files Saved ${GOOD}:-)${NORMAL}"
    rm ${TMPDIR}/${TMPTARFILE}
    echo
    
    if [ -n "$OLDSIZE" ] ; then
	fsize ${MNTDIR}/${TARFILE}
	NEWSIZE=$size

	fverb "Old tarball size : ${BOLD}${OLDSIZE}kb${NORMAL}"
	fverb "New Tarball size : ${BOLD}${NEWSIZE}kb${NORMAL}"
	echo
    fi
    
    ##
    # Md5 creation (new in 0.4)
    ##
    MD5TAR=$(md5sum ${MNTDIR}/${TARFILE})
    [ -f "${MD5BAK}" ] && MD5BAK=$(md5sum ${MNTDIR}/${BAKFILE})
    echo ${MD5TAR} > ${MNTDIR}/${MD5FILE}
    [ -n "${MD5BAK}" ] && echo ${MD5BAK} >> ${MNTDIR}/${MD5FILE}
    fverb "Md5 file created"
    echo
    
    fumount
    
fi

# vim: set tabstop=8 shiftwidth=4: 
