#!/usr/local/bin/cbsd
# Wrapper for creating debootstrap environvent via 2 phases:
# 1) Get distribution into skel dir from FTP
# 2) Get distribution into data dir from skel dir

. ${subr}
. ${cbsdinit}

. /etc/rc.conf

if [ -z "${cbsd_workdir}" ]; then
	echo "No workdir"
	exit 1
fi

workdir="${cbsd_workdir}"

[ ! -f "${distdir}/cbsd.conf" ] && exit 1
. ${distdir}/cbsd.conf
. ${subr}
. ${system}
. ${strings}
. ${tools}

SRC_MIRROR="http://deb.debian.org/debian/"
customskel="${sharedir}/FreeBSD-jail-debian-buster-system-skel"

[ -z "${jname}" ] && err 1 "${N1_COLOR}Empty jname${N0_COLOR}"

[ ! -d ${customskel} ] && ${MKDIR_CMD} -p ${customskel}

if [ ! -x /usr/local/sbin/debootstrap ]; then
	err 1 "${N1_COLOR}No such debootstrap. Please ${N2_COLOR}pkg install debootstrap${N1_COLOR} it.${N0_COLOR}"
fi

for module in linprocfs fdescfs tmpfs linsysfs; do
	${KLDSTAT_CMD} -m "${module}" > /dev/null 2>&1 || ${KLDLOAD_CMD} ${module}
done

${KLDSTAT_CMD} -m linuxelf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux
${KLDSTAT_CMD} -m linux64elf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux64

if [ ! -f ${customskel}/bin/bash ]; then
	export INTER=1

	if getyesno "Shall i download distribution via deboostrap from ${SRC_MIRROR}?"; then
		${ECHO} "${N1_COLOR}debootstrap ${H5_COLOR}--include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib ${H3_COLOR}buster ${N1_COLOR}${customskel} ${SRC_MIRROR}${N0_COLOR}"
		/bin/sh <<EOF
/usr/local/sbin/debootstrap --include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib --arch=amd64 --no-check-gpg buster ${customskel} ${SRC_MIRROR}
EOF
	else
		echo "No such distribution"
		exit 1
	fi
fi

[ ! -f ${customskel}/bin/bash ] && err 1 "${N1_COLOR}No such distribution on ${N2_COLOR}${customskel}${N0_COLOR}"

. ${jrcconf}
[ "${baserw}" = "1" ] && path=${data}

if [ ! -d ${data}/bin/bash ]; then
	. ${distdir}/freebsd_world.subr
	customskel
fi

[ ! -f ${data}/bin/bash ] && err 1 "${N1_COLOR}No such ${data}/bin/bash"

printf "APT::Cache-Start 251658240;" > ${customskel}/etc/apt/apt.conf.d/00freebsd
cat > ${customskel}/etc/apt/sources.list <<EOF
deb http://deb.debian.org/debian buster main
deb-src http://deb.debian.org/debian buster main
deb http://security.debian.org/ buster/updates main
deb-src http://security.debian.org/ buster/updates main
deb http://deb.debian.org/debian buster-updates main
deb-src http://deb.debian.org/debian buster-updates main
deb http://deb.debian.org/debian buster-backports main
deb-src http://deb.debian.org/debian buster-backports main
EOF


exit 0
