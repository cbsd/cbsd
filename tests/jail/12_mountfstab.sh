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

jname="jcreate1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate runasap=0 jname="${jname}" pkg_bootstrap=0
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}


test_create_extrafstab()
{
	${CIX_BIN} jstart jname="${jname}"

	cat > ~cbsd/jails-fstab/${jname}/fstab.local <<EOF
/COPYRIGHT /var/TEST nullfs ro 0 0
EOF

	echo "extra fstab:"
	cat ~cbsd/jails-fstab/${jname}/fstab.local

	${CIX_BIN} mountfstab jname=${jname}

	_test=$( jexec "${jname}" realpath /var/TEST )

	assertEquals "${_test}" "/var/TEST"
}

. ${progdir}/../shunit2
