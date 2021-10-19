#!/usr/local/bin/cbsd
# Wrapper for creating debootstrap environvent via 2 phases:
# 1) Get distribution into skel dir from FTP
# 2) Get distribution into data dir from skel dir

. "${subrdir}"/nc.subr
. "${cbsdinit}"

. /etc/rc.conf

if [ -z "${cbsd_workdir}" ]; then
	echo "No workdir"
	exit 1
fi

workdir="${cbsd_workdir}"

[ ! -f "${distdir}/cbsd.conf" ] && exit 1
. "${distdir}"/cbsd.conf
. "${subrdir}"/nc.subr
. "${system}"
. "${strings}"
. "${tools}"

SRC_MIRROR="http://ftp.us.debian.org/debian"
# + http://deb.debian.org/debian/
customskel="${sharedir}/FreeBSD-jail-kfreebsd-wheezy-skel"

[ -z "${jname}" ] && err 1 "${N1_COLOR}Empty jname${N0_COLOR}"

[ ! -d "${customskel}" ] && ${MKDIR_CMD} -p "${customskel}"

if [ ! -x /usr/local/sbin/debootstrap ]; then
	err 1 "${N1_COLOR}No such debootstrap. Please ${N2_COLOR}pkg install debootstrap${N1_COLOR} it.${N0_COLOR}"
fi

for module in linprocfs fdescfs tmpfs linsysfs; do
	${KLDSTAT_CMD} -m "${module}" > /dev/null 2>&1 || ${KLDLOAD_CMD} ${module}
done

if [ ! -r "${customskel}"/bin/bash ]; then
	export INTER=1

	if getyesno "Shall i download distribution via deboostrap from ${SRC_MIRROR}?"; then
		${ECHO} "${N1_COLOR}debootstrap ${H5_COLOR}--include=openssh-server,locales,joe,rsync,sharutils,psmisc,htop,patch,less,apt --components main,contrib ${H3_COLOR}wheezy ${N1_COLOR}${customskel} ${SRC_MIRROR}${N0_COLOR}"
		debootstrap --include=openssh-server,locales,joe,rsync,sharutils,psmisc,htop,patch,less,apt --components main,contrib wheezy "${customskel}" ${SRC_MIRROR}
		${CHROOT_CMD} "${customskel}" dpkg -i /var/cache/apt/archives/*.deb
	else
		echo "No such distribution"
		exit 1
	fi
fi

[ ! -f "${customskel}"/bin/bash ] && err 1 "${N1_COLOR}No such distribution on ${N2_COLOR}${customskel}${N0_COLOR}"

. "${subrdir}"/rcconf.subr
[ "${baserw}" = "1" ] && path=${data}

if [ ! -d "${data}"/bin/bash ]; then
	. "${subrdir}"/freebsd_world.subr
	customskel
fi

[ ! -f "${data}"/bin/bash ] && err 1 "${N1_COLOR}No such ${data}/bin/bash"
exit 0
