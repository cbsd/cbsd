#!/usr/local/bin/cbsd
# Wrapper for creating environvent via 2 phases:
# 1) Get distribution into skel dir from repo
# 2) Get distribution into data dir from skel dir

VERSION_CODENAME="daedalus"

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

[ -z "${arch}" ] && arch=$( ${UNAME_CMD} -m )

rootfs_dir="${basejaildir}/base_${arch}_${arch}_${VERSION_CODENAME}"

if [ ! -d ${rootfs_dir}/etc ]; then
	repo action=get sources=base arch=${arch} ver=${VERSION_CODENAME} platform=Linux
fi

[ ! -r ${rootfs_dir}/bin/bash ] && err 1 "${N1_COLOR}no such basejail in ${rootfs_dir}, failed: ${N2_COLOR}repo action=get sources=base arch=${arch} ver=${VERSION_CODENAME} platform=Linux${N0_COLOR}"

for module in linprocfs fdescfs tmpfs linsysfs; do
	${KLDSTAT_CMD} -m "${module}" > /dev/null 2>&1 || ${KLDLOAD_CMD} ${module}
done

${KLDSTAT_CMD} -m linux_common > /dev/null 2>&1 || ${KLDLOAD_CMD} linux_common
${KLDSTAT_CMD} -m linux64elf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux64

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
