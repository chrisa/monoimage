#!/bin/bash

#GNAP_OVERLAY="/home/software/unpacked/gnap2/gnap-tools-2.0rc1/gnap_overlay_dunc"
#CORE_FILE="/home/dunc/work/suppose-gnap/gnap-2.0rc1-20060410.tar"
#OUT_DIR="/home/dunc/work/suppose-gnap/out"
#VPN="vpn1"


usage() {
        echo 'Options:'
        echo '    -h                   This Help'
        echo '    -o output_dir        Directory to place output'
        echo '    -d base_dir          base directory for overlay files'
        echo '    -b overlay_binary    Which gnap_overlay file to use'
        echo '    -c core_file         GNAP core file to bash image on'
        echo '    -v vpn               Name of VPN (which filesystem overlay to use)'
        echo '    -n nodename          Name of node to build image for (which filesystem overlay to use)'
}

test_options() {
	if [[ ! -d ${BASE_DIR} ]]; then
		echo "base_dir ${BASE_DIR} is not a directory"
		exit 1
	fi
	if [[ ! -f ${GNAP_OVERLAY} ]]; then
		echo "Cannon find gnap_overlay binary ${GNAP_OVERLAY}"
		exit 1
	fi
	if [[ ! -f ${CORE_FILE} ]]; then
		echo "Cannon find gnap core file ${CORE_FILE}"
		exit 1
	fi
	if [[ -z ${VPN} ]]; then
		echo "VPN not specified"
		exit 1
	fi
	if [[ -z ${NODENAME} ]]; then
		echo "nodename not specified"
		exit 1
	fi

	if [[ ! -d ${BASE_DIR}/${VPN} ]]; then
		echo "cannot find base directory ${BASE_DIR}/${VPN} for VPN"
		exit 1
	fi
	if [[ ! -d ${BASE_DIR}/${VPN}/${NODENAME} ]]; then
		echo "cannot find overlay directory ${BASE_DIR}/${VPN}/${NODENAME} for node ${NODENAME}"
		exit 1
	fi


}

while getopts ':o:hc:v:b:n:d:' options; do
        case ${options} in
                h ) usage
                        exit 0;;
                d ) BASE_DIR="${OPTARG}";;
                b ) GNAP_OVERLAY="${OPTARG}";;
                c ) CORE_FILE="${OPTARG}";;
                n ) NODENAME="${OPTARG}";;
                v ) VPN="${OPTARG}";;
                * ) echo 'Unknown options specified'
			usage
			exit 0;;
        esac
done

test_options


echo
echo "Running ${GNAP_OVERLAY} -g ${CORE_FILE} -o ${BASE_DIR}/common -o ${BASE_DIR}/${VPN}/common -o ${BASE_DIR}/${VPN}/${NODENAME} -i gnap.iso"

${GNAP_OVERLAY} -g ${CORE_FILE} -o ${BASE_DIR}/common -o ${BASE_DIR}/${VPN}/common -o ${BASE_DIR}/${VPN}/${NODENAME} -i gnap.iso

