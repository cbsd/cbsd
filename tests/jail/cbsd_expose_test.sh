#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="test"

oneTimeSetUp()
{
	if ${CIX_BIN} jstatus jname="${jname}"; then
		${CIX_BIN} jcreate jname="${jname}"
	fi
}

setUp()
{
	${CIX_BIN} expose jname=${jname} mode=flush
}

tearDown()
{
	${CIX_BIN} expose jname=${jname} mode=flush
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
