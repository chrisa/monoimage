#!/bin/bash

#GNAP_MAKE="/home/software/unpacked/gnap2/gnap-2.0rc1/gnap_make"
#PROFILE="std"
#PORTAGE_SNAPSHOT="portage-latest.tar.bz2"
#OVERLAY_DIR="/home/dunc/work/suppose-gnap/portage-overlay"

usage() {
        echo 'Options:'
        echo '    -h                   This Help'
        echo '    -b gnap_make_binary  Which gnap_make file to use'
        echo '    -p profile           Which profile to use from the specs dir'
        echo '    -s snapshot	       Portage snapshot file to use'
        echo '    -o portage_overlay   Portage overlay directory to use'
}

test_options() {
	if [[ ! -f ${GNAP_MAKE} ]]; then
		echo "Cannon find gnap_make binary ${GNAP_MAKE}"
		exit 1
	fi
	if [[ ! -f ${PORTAGE_SNAPSHOT} ]]; then
		echo "Cannot find portage snapshot file ${PORTAGE_SNAPSHOT}"
		exit 1
	fi
	if [[ ! -z ${OVERLAY_DIR} ]]; then
		if [[ ! -d ${OVERLAY_DIR} ]]; then
			echo "overlay dir ${OVERLAY_DIR} is not a directory"
			exit 1
		fi
	fi
	if [[ -z ${PROFILE} ]]; then
		echo "Profile not specified"
		exit 1
	fi

	if [[ ! -d specs/${PROFILE} ]]; then
		echo "cannot find specs directory specs/${PROFILE}"
		exit 1
	fi
}

while getopts ':b:hp:s:o:' options; do
        case ${options} in
                h ) usage
                        exit 0;;
                b ) GNAP_MAKE="${OPTARG}";;
                p ) PROFILE="${OPTARG}";;
                s ) PORTAGE_SNAPSHOT="${OPTARG}";;
                o ) OVERLAY_DIR="${OPTARG}";;
                * ) echo 'Unknown options specified'
			usage
			exit 0;;
        esac
done

test_options


echo "Running ${GNAP_MAKE} -t all -o ${OVERLAY_DIR} -p ${PORTAGE_SNAPSHOT} -e specs/${PROFILE}"

${GNAP_MAKE} -t all -o ${OVERLAY_DIR} -p ${PORTAGE_SNAPSHOT} -e specs/${PROFILE}

