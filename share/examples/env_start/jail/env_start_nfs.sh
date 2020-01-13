#!/bin/sh
# CBSD Project, custom script for mnt_start hook sample ( NFSv4 client )

# read config from /root/etc/env_start_nfs.conf if file exist
# see /usr/local/cbsd/share/examples/env_start/jail/env_start_nfs.conf as sample
[ -r /root/etc/env_start_nfs.conf ] && . /root/etc/env_start_nfs.conf

# NFS server 127.0.0.1 as default: change this ( e.g via /root/etc/env_start.nfs.conf
# see /usr/local/cbsd/share/examples/env_start/jail/env_start_nfs.conf as sample
: ${NFS_SERVER:=127.0.0.1}
: ${NFS_SERVER_ROOT_DIR:=/nfs}
: ${MOUNT_RCCONF:=0}

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

print_info()
{
	echo "[debug] server: ${NFS_SERVER}"
	echo "[debug] cmd: ${cmd}"
	echo "[debug] errcode: ${ret}"
	echo "[debug] remote folder: ${remote_dir} - remote dir exist ?"
	exit ${ret}
}


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

trap "print_info" HUP INT ABRT BUS TERM EXIT
ret=0

if is_mounted ${data}; then
	echo "[debug] ${data} already mounted!" 1>&2
else
	remote_dir="${NFS_SERVER_DATA_DIR}/${jname}"
	cmd="timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${remote_dir} ${data}"
	${cmd}
	ret=$?
fi

[ ${ret} -ne 0 ] && exit ${ret}

if is_mounted ${sysdata}; then
	echo "[debug] ${sysdata} already mounted!" 1>&2
else
	remote_dir="${NFS_SERVER_SYSTEM_DIR}/${jname}"
	cmd="timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${remote_dir} ${sysdata}"
	${cmd}
	ret=$?
fi

[ ${ret} -ne 0 ] && exit ${ret}

# mount rcconf if necessary, see config file
if [ ${MOUNT_RCCONF} -eq 1 ]; then
	if is_mounted ${rcconf}; then
		echo "[debug] ${rcconf} already mounted!" 1>&2
	else
		remote_dir="${NFS_SERVER_RCCONF_DIR}/${jname}"
		cmd="timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${remote_dir} ${rcconf}"
		${cmd}
		ret=$?
	fi
	[ ${ret} -ne 0 ] && exit ${ret}
fi

if is_mounted ${fstab}; then
	echo "[debug] ${fstab} already mounted!" 1>&2
else
	remote_dir="${NFS_SERVER_FSTAB_DIR}/${jname}"
	cmd="timeout ${NFS_MOUNT_TIMEOUT} mount_nfs -orw -overs=4 ${NFS_SERVER}:${remote_dir} ${fstab}"
	${cmd}
	ret=$?
fi

# clean/unset trap
trap "" HUP INT ABRT BUS TERM EXIT
exit ${ret}
