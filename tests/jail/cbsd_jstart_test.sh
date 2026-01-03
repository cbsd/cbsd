#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="cbsd_test_jstart"

oneTimeSetUp()
{
	${CIX_BIN} jdestroy jname="${jname}"
}

setUp()
{
	${CIX_BIN} jcreate runasap=0 jname="${jname}"
}

tearDown()
{
	${CIX_BIN} jdestroy jname="${jname}"
}

# just test if jstart works at all
test_jstart()
{
	${CIX_BIN} jstart jname="${jname}"
	_test=$(jexec "${jname}" whoami)
	assertEquals "${_test}" "root"
}

# change hostname from inside jail
test_issue649_2()
{
	${CIX_BIN} jstart jname="${jname}"
	${CIX_BIN} jexec jname="${jname}" sysrc hostname="lollipop"
	${CIX_BIN} jstop jname="${jname}"
	${CIX_BIN} jstart jname="${jname}"
	_test=$(${CIX_BIN} jexec jname="${jname}" hostname)
	assertEquals "${_test}" "lollipop"
}

. ${progdir}/../shunit2
