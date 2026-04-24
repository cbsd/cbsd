#!/bin/sh
# Scenario:
#  create $jname jail
#  - modify jail.fstab for extra mount
#  execute:
#    ${CIX_BIN} mountfstab jname=$jname
#  check for additional mouns
# TODO : check for external script mount, e.g. fusefs-s3/ntfs..
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

# get workdir
. /etc/rc.conf

jname="jcreate1"

CBSD_JAIL_ATTACH="zroot/12_mountfstab_zfs_test"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate runasap=0 jname="${jname}" pkg_bootstrap=0 allow_zfs=1 sysrc="zfs_enable=YES"
	zfs list ${CBSD_JAIL_ATTACH} >/dev/null 2>&1 && zfs destroy ${CBSD_JAIL_ATTACH}
	zfs create ${CBSD_JAIL_ATTACH}
	_ret=$?
	assertEquals "failed: zfs create ${CBSD_JAIL_ATTACH}" 0 "${_ret}"
	zfs set mountpoint=none ${CBSD_JAIL_ATTACH}
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	zfs list ${CBSD_JAIL_ATTACH} >/dev/null 2>&1 && zfs destroy ${CBSD_JAIL_ATTACH}
	return 0
}

test_create_extrafstab()
{
	local _ret=

	cat > ${cbsd_workdir}/jails-fstab/${jname}/fstab.local <<EOF
${CBSD_JAIL_ATTACH} /usr/local/mnt zfs rw 0 0
EOF

	echo "extra fstab:"
	cat ${cbsd_workdir}/jails-fstab/${jname}/fstab.local

	${CIX_BIN} jstart jname="${jname}"

	_test=$( jexec "${jname}" zfs get -Ho value mountpoint ${CBSD_JAIL_ATTACH} )
	assertEquals "${_test}" "/usr/local/mnt"
}

. ${progdir}/../shunit2
