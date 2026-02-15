#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

oneTimeSetUp()
{
	jname="jcreate1"
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate jname="${jname}" etcupdate_init=0 pkg_bootstrap=0 quiet=1
}

setUp()
{
	${CIX_BIN} expose jname=${jname} mode=flush
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

test_expose_delete()
{
	${CIX_BIN} expose mode=add jname="${jname}" in=37684 out=37684
	test=$( ${CIX_BIN} expose mode=list jname="${jname}" | grep 37684 )
	assertNotNull "Expose was not added" "${test}"

	${CIX_BIN} expose mode=delete jname="${jname}" in=37684 out=37684
	test=$( ${CIX_BIN} expose mode=list jname="${jname}" | grep 37684 )
	assertNull "Expose was not deleted" "${test}"
}

. ${progdir}/../shunit2
