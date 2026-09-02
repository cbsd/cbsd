#!/usr/bin/env cbsd
# Wrapper for creating environvent via 2 phases:
# 1) Get distribution into skel dir from repo
# 2) Get distribution into data dir from skel dir
VERSION_CODENAME="excalibur"

. ${subrdir}/nc.subr
cixinit
[ -z "${CIX_DISTDIR}" ] && CIX_DISTDIR="/usr/local/cbsd"
unset workdir

# MAIN
[ -z "${cbsd_workdir}" ] && . /etc/rc.conf
if [ -z "${cbsd_workdir}" ]; then
	echo "No workdir"
	exit 1
else
	workdir="${cbsd_workdir}"
fi
[ ! -r "${CIX_DISTDIR}/cbsd.conf" ] && exit 1

. ${CIX_DISTDIR}/cbsd.conf
. ${subrdir}/nc.subr
. ${system}
. ${strings}
. ${tools}

[ -z "${arch}" ] && capture arch ${UNAME_CMD} -m

rootfs_dir="${basejaildir}/base_${arch}_${arch}_${VERSION_CODENAME}"

if [ ! -d ${rootfs_dir}/etc ]; then
	set -o xtrace
	repo action=get sources=base arch=${arch} ver=${VERSION_CODENAME} platform=Linux
	set +o xtrace
fi

[ ! -r ${rootfs_dir}/bin/bash ] && err 1 "${N1_COLOR}no such basejail in ${rootfs_dir}, failed: ${N2_COLOR}repo action=get sources=base arch=${arch} ver=${VERSION_CODENAME} platform=Linux${N0_COLOR}"

for module in linux64 linux_common linprocfs fdescfs tmpfs linsysfs; do
	kernel_mod -l ${module}
done

. ${subrdir}/rcconf.subr
[ "${baserw}" = "1" ] && path="${data}"

if [ ! -r ${data}/bin/bash ]; then
	${ECHO} "${N1_COLOR}populate jails data from: ${N2_COLOR}${rootfs_dir} ...${N0_COLOR}"
	# populate jails data from rootfs?
	. ${subrdir}/freebsd_world.subr
	echo "customskel -s ${rootfs_dir}"
	customskel -s ${rootfs_dir}
fi

if [ -d "${CIX_DISTDIR}/share/Linux-jail-devuan-${VERSION_CODENAME}-skel" -a ! -r "${path}/etc/cix_bootstrapped" ]; then
	${RSYNC_CMD} -avz ${CIX_DISTDIR}/share/Linux-jail-devuan-${VERSION_CODENAME}-skel/ ${path}/
	${TOUCH_CMD} ${path}/etc/cix_bootstrapped
fi

[ ! -f ${data}/bin/bash ] && err 1 "${N1_COLOR}No such ${data}/bin/bash"

exit 0
