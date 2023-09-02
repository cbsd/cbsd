#!/usr/local/bin/cbsd
# Wrapper for creating Ubuntu environvent via 2 phases:
# 1) Get distribution into skel dir from FTP
# 2) Get distribution into data dir from skel dir

. ${subrdir}/nc.subr
. ${cbsdinit}
: ${distdir="/usr/local/cbsd"}
unset workdir

# MAIN
[ -z "${cbsd_workdir}" ] && . /etc/rc.conf
if [ -z "${cbsd_workdir}" ]; then
	echo "No workdir"
	exit 1
else
	workdir="${cbsd_workdir}"
fi
[ ! -r "${distdir}/cbsd.conf" ] && exit 1

. ${distdir}/cbsd.conf
. ${subrdir}/nc.subr
. ${system}
. ${strings}
. ${tools}
. ${subrdir}/fetch.subr


SRC_MIRROR="\
http://archive.ubuntu.com/ubuntu/
https://mirror.internet.asn.au/pub/ubuntu/archive/ \
http://mirror.easyname.at/ubuntu-archive/ \
https://mirrors.edge.kernel.org/ubuntu/ \
http://ftp.halifax.rwth-aachen.de/ubuntu/ \
https://mirror.xtom.com.hk/ubuntu/ \
http://mirror.nl.datapacket.com/ubuntu/ \
http://mirror.enzu.com/ubuntu/ \
"

rootfs_dir="${sharedir}/jail-ubuntu-jammy-rootfs"

[ -z "${jname}" ] && err 1 "${N1_COLOR}empty jname${N0_COLOR}"
[ ! -d ${rootfs_dir} ] && ${MKDIR_CMD} -p ${rootfs_dir}

BASH_CMD=$( which bash 2>/dev/null )
DEBOOTSTRAP_CMD="/usr/local/sbin/debootstrap"

if [ ! -x ${BASH_CMD} ]; then
	err 1 "${N1_COLOR}No such bash executable. Please ${N2_COLOR}pkg install -y bash${N1_COLOR} it.${N0_COLOR}"
fi
if [ ! -r ${DEBOOTSTRAP_CMD} ]; then
	err 1 "${N1_COLOR}No such debootstrap. Please ${N2_COLOR}pkg install -y debootstrap${N1_COLOR} it.${N0_COLOR}"
fi

for module in linprocfs fdescfs tmpfs linsysfs; do
	${KLDSTAT_CMD} -m "${module}" > /dev/null 2>&1 || ${KLDLOAD_CMD} ${module}
done

${KLDSTAT_CMD} -m linuxelf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux
${KLDSTAT_CMD} -m linux64elf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux64

if [ ! -f ${rootfs_dir}/bin/bash ]; then
	export INTER=1

	if getyesno "Shall i download distribution via deboostrap from ${SRC_MIRROR}?"; then

		${ECHO} "${N1_COLOR}Scanning for fastest mirror...${N0_COLOR}"
		scan_fastest_mirror -s "${SRC_MIRROR}" -t 2 -u "dists/jammy/Contents-amd64.gz"
		for i in ${FASTEST_SRC_MIRROR}; do
			${ECHO} "${N1_COLOR}${BASH_CMD} ${DEBOOTSTRAP_CMD} ${H5_COLOR}--include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib ${H3_COLOR}jammy ${N1_COLOR}${rootfs_dir} ${i}${N0_COLOR}"
			/bin/sh <<EOF
${BASH_CMD} ${DEBOOTSTRAP_CMD} --include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib --arch=amd64 --no-check-gpg jammy ${rootfs_dir} ${i}
EOF
			ret=$?
			[ ${ret} -eq 0 ] && break
		done
		printf "APT::Cache-Start 251658240;" > ${rootfs_dir}/etc/apt/apt.conf.d/00freebsd
		${CAT_CMD} > ${rootfs_dir}/etc/apt/sources.list <<EOF
deb http://archive.ubuntu.com/ubuntu jammy main restricted
deb http://archive.ubuntu.com/ubuntu jammy multiverse
deb http://archive.ubuntu.com/ubuntu jammy universe
deb http://archive.ubuntu.com/ubuntu jammy-backports main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu jammy-updates main restricted
deb http://archive.ubuntu.com/ubuntu jammy-updates multiverse
deb http://archive.ubuntu.com/ubuntu jammy-updates universe
deb http://security.ubuntu.com/ubuntu jammy-security main restricted
deb http://security.ubuntu.com/ubuntu jammy-security multiverse
deb http://security.ubuntu.com/ubuntu jammy-security universe
EOF

	else
		echo "canceled"
		exit 1
	fi
fi

[ ! -f ${rootfs_dir}/bin/bash ] && err 1 "${N1_COLOR}No such distribution in ${N2_COLOR}${rootfs_dir}${N0_COLOR}"

. ${subrdir}/rcconf.subr
[ "${baserw}" = "1" ] && path=${data}

if [ ! -r ${data}/bin/bash ]; then
	${ECHO} "${N1_COLOR}populate jails data from: ${N2_COLOR}${rootfs_dir} ...${N0_COLOR}"
	# populate jails data from rootfs?
	. ${subrdir}/freebsd_world.subr
	customskel -s ${rootfs_dir}
fi

[ ! -f ${data}/bin/bash ] && err 1 "${N1_COLOR}No such ${data}/bin/bash"

exit 0
