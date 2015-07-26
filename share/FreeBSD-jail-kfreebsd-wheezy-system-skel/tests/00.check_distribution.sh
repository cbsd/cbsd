#!/usr/local/bin/cbsd
# Wrapper for creating debootstrap environvent via 2 phases:
# 1) Get distribution into skel dir from FTP
# 2) Get distribution into data dir from skel dir

. /etc/rc.conf

if [ -z "${cbsd_workdir}" ]; then
	echo "No workdir"
	exit 1
fi

workdir="${cbsd_workdir}"

[ ! -f "${workdir}/cbsd.conf" ] && exit 1
. ${workdir}/cbsd.conf
. ${subr}
. ${system}
. ${strings}
. ${tools}

SRC_MIRROR="http://ftp.us.debian.org/debian"
customskel="${sharedir}/FreeBSD-jail-kfreebsd-wheezy-skel"

[ -z "${jname}" ] && err 1 "${MAGENTA}Empty jname${NORMAL}"

if [ ! -x /usr/local/sbin/debootstrap ]; then
	err 1 "${MAGENTA}No such debootstrap. Please ${GREEN}pkg install debootstrap${MAGENTA} it.${NORMAL}"
fi

for module in linprocfs fdescfs tmpfs linsysfs; do
	/sbin/kldstat -m "$module" > /dev/null 2>&1 || /sbin/kldload ${module}
done

if [ ! -f ${customskel}/bin/bash ]; then
	export INTER=1

	if getyesno "Shall i download distribution via deboostrap from ${SRC_MIRROR}?"; then
		debootstrap wheezy ${customskel} ${SRC_MIRROR}
	else
		echo "No such distribution"
		exit 1
	fi
fi

[ ! -f ${customskel}/bin/bash ] && err 1 "${MAGENTA}No such distribution on ${GREEN}${customskel}${NORMAL}"

. ${jrcconf}
[ "$baserw" = "1" ] && path=${data}

. ${workdir}/freebsd_world.subr
customskel

[ ! -f ${data}/bin/bash ] && err 1 "${MAGENTA}No such ${data}/bin/bash"
exit 0

