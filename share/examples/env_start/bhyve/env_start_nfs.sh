#!/bin/sh
# CBSD Project

# read config from /root/etc/env_start_nfs.conf if file exist
# see /usr/local/cbsd/share/examples/env_start/jail/env_start_nfs.conf as sample
[ -r /root/etc/env_start_nfs.conf ] && . /root/etc/env_start_nfs.conf

# NFS server 127.0.0.1 as default: change this ( e.g via /root/etc/env_start.nfs.conf
# see /usr/local/cbsd/share/examples/env_start/jail/env_start_nfs.conf as sample
: ${NFS_SERVER:=127.0.0.1}
: ${NFS_SERVER_ROOT_DIR:=/nfs}

[ -z "${NFS_SERVER_DATA_DIR}" ] && NFS_SERVER_DATA_DIR="${NFS_SERVER_ROOT_DIR}/data"
[ -z "${NFS_SERVER_RCCONF_DIR}" ] && NFS_SERVER_RCCONF_DIR="${NFS_SERVER_ROOT_DIR}/rcconf"
[ -z "${NFS_SERVER_FSTAB_DIR}" ] && NFS_SERVER_FSTAB_DIR="${NFS_SERVER_ROOT_DIR}/fstab"
[ -z "${NFS_SERVER_SYSTEM_DIR}" ] && NFS_SERVER_SYSTEM_DIR="${NFS_SERVER_ROOT_DIR}/system"

[ -z "${NFS_MOUNT_TIMEOUT}" ] && NFS_MOUNT_TIMEOUT="10"

data=
fstab=
jname=
rcconf=
sysdata=

# check for mounted resouces
# $1 - directory
# e.g:
# if is_mounted /tmp; then ...mounted..
# or
# if is_mounted zroot/ROOT; then ..mounted..
is_mounted()
{
	local _tst

	# test for destrination is directory ?
	if [ -d "${1}" ]; then
		#_tst=$( df -t nfs ${1} | tail +2 | awk '{ print $6 }' )
		_tst=$( df ${1} | tail +2 | awk '{ print $6 }' )
		[ "${_tst}" = "${1}" ] && return 0
	fi

	return 1
}

# MAIN()
while getopts "d:f:j:r:s:" opt; do
	case "${opt}" in
		d) data="${OPTARG}" ;;
		f) fstab="${OPTARG}" ;;
		j) jname="${OPTARG}" ;;
		r) rcconf="${OPTARG}" ;;
		s) sysdata="${OPTARG}" ;;
	esac
	shift $(( ${OPTIND} - 1 ))
done


if [ -z "${data}" ]; then
	echo "Empty data, use -d <datadir>"
fi
if [ -z "${jname}" ]; then
	echo "Empty jname, use -j <jname>"
fi
if [ -z "${sysdata}" ]; then
	echo "Empty sysdata, use -s <sysdata>"
fi

echo "Mount data for jname: ${jname}"
echo "data: ${data}"
echo "sysdata: ${sysdata}"

ret=0

if is_mounted ${data}; then
	echo "${data} already mounted!"
else
	timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${NFS_SERVER_DATA_DIR}/${jname} ${data}
	ret=$?
fi

[ ${ret} -ne 0 ] && exit ${ret}

if is_mounted ${sysdata}; then
	echo "${sysdata} already mounted!"
else
	timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${NFS_SERVER_SYSTEM_DIR}/${jname} ${sysdata}
	ret=$?
fi

[ ${ret} -ne 0 ] && exit ${ret}

if is_mounted ${rcconf}; then
	echo "${rcconf} already mounted!"
else
	timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${NFS_SERVER_RCCONF_DIR}/${jname} ${rcconf}
	ret=$?
fi

[ ${ret} -ne 0 ] && exit ${ret}

if is_mounted ${fstab}; then
	echo "${fstab} already mounted!"
else
	timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${NFS_SERVER_FSTAB_DIR}/${jname} ${fstab}
	ret=$?
fi

exit ${ret}
