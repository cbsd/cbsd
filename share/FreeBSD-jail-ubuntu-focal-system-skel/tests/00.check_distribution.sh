#!/usr/local/bin/cbsd
# Wrapper for creating debootstrap environvent via 2 phases:
# 1) Get distribution into skel dir from FTP
# 2) Get distribution into data dir from skel dir

. "${subrdir}"/nc.subr
. "${cbsdinit}"
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
. "${subrdir}"/nc.subr
. "${system}"
. "${strings}"
. "${tools}"
. "${subrdir}"/fetch.subr


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

rootfs_dir="${sharedir}/jail-ubuntu-focal-rootfs"

[ -z "${jname}" ] && err 1 "${N1_COLOR}empty jname${N0_COLOR}"
[ ! -d "${rootfs_dir}" ] && ${MKDIR_CMD} -p "${rootfs_dir}"

if [ ! -x /usr/local/sbin/debootstrap ]; then
	err 1 "${N1_COLOR}No such debootstrap. Please ${N2_COLOR}pkg install debootstrap${N1_COLOR} it.${N0_COLOR}"
fi

for module in linprocfs fdescfs tmpfs linsysfs; do
	${KLDSTAT_CMD} -m "${module}" > /dev/null 2>&1 || ${KLDLOAD_CMD} ${module}
done

${KLDSTAT_CMD} -m linuxelf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux
${KLDSTAT_CMD} -m linux64elf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux64

if [ ! -f "${rootfs_dir}"/bin/bash ]; then
	export INTER=1

	if getyesno "Shall i download distribution via deboostrap from ${SRC_MIRROR}?"; then

		${ECHO} "${N1_COLOR}Scanning for fastest mirror...${N0_COLOR}"
		scan_fastest_mirror -s "${SRC_MIRROR}" -t 2 -u "dists/focal/Contents-amd64.gz"
		for i in ${FASTEST_SRC_MIRROR}; do
			${ECHO} "${N1_COLOR}debootstrap ${H5_COLOR}--include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib ${H3_COLOR}focal ${N1_COLOR}${rootfs_dir} ${i}${N0_COLOR}"
			/bin/sh <<EOF
/usr/local/sbin/debootstrap --include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib --arch=amd64 --no-check-gpg focal ${rootfs_dir} ${i}
EOF
			ret=$?
			[ ${ret} -eq 0 ] && break
		done
		printf "APT::Cache-Start 251658240;" > "${rootfs_dir}"/etc/apt/apt.conf.d/00freebsd
		${CAT_CMD} > "${rootfs_dir}"/etc/apt/sources.list <<EOF
deb http://archive.ubuntu.com/ubuntu focal main restricted
deb http://archive.ubuntu.com/ubuntu focal multiverse
deb http://archive.ubuntu.com/ubuntu focal universe
deb http://archive.ubuntu.com/ubuntu focal-backports main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu focal-updates main restricted
deb http://archive.ubuntu.com/ubuntu focal-updates multiverse
deb http://archive.ubuntu.com/ubuntu focal-updates universe
deb http://security.ubuntu.com/ubuntu focal-security main restricted
deb http://security.ubuntu.com/ubuntu focal-security multiverse
deb http://security.ubuntu.com/ubuntu focal-security universe
EOF

	else
		echo "canceled"
		exit 1
	fi
fi

[ ! -f "${rootfs_dir}"/bin/bash ] && err 1 "${N1_COLOR}No such distribution in ${N2_COLOR}${rootfs_dir}${N0_COLOR}"

. "${subrdir}"/rcconf.subr
[ "${baserw}" = "1" ] && path=${data}

if [ ! -r "${data}"/bin/bash ]; then
	${ECHO} "${N1_COLOR}populate jails data from: ${N2_COLOR}${rootfs_dir} ...${N0_COLOR}"
	# populate jails data from rootfs?
	. "${subrdir}"/freebsd_world.subr
	customskel -s "${rootfs_dir}"
fi

[ ! -f "${data}"/bin/bash ] && err 1 "${N1_COLOR}No such ${data}/bin/bash"

exit 0
