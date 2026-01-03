#!/bin/sh
# Scenario:
#  create empty jail
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

oneTimeSetUp()
{
	jname=jcreate1
}

setUp()
{
	dir=$( mktemp -d )
	cd "${dir}" || exit 1
}

tearDown()
{
	${CIX_BIN} jremove jname="${jname}"
	rm -rf "${dir}"
}

testFreeBSDVersion()
{
	${CIX_BIN} jcreate jname="${jname}" baserw=1 ver=empty applytpl=0 mount_devfs=0 mount_ports=0 mount_src=0 floatresolv=0 etcupdate_init=0 pkg_bootstrap=0
	is_empty=$( ls -1 ~cbsd/jails-data/${jname}-data/ )
	assertEquals "Jail FreeBSD version" "${jail_version}" ""
}

. ${progdir}/../shunit2
