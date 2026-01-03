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

# https://github.com/cbsd/cbsd/issues/649
# change host_hostname
test_issue649_0()
{
	${CIX_BIN} jset jname="${jname}" host_hostname="one"
	_test=$( ${CIX_BIN} jget jname="${jname}" mode=quiet host_hostname )
	assertEquals "one" "${_test}"
}

# change host_hostname on-the-fly
test_issue649_1()
{
	${CIX_BIN} jstart jname="${jname}"
	${CIX_BIN} jset jname="${jname}" host_hostname="two"
	${CIX_BIN} jrestart jname="${jname}"
	_test=$( ${CIX_BIN} jget jname="${jname}" mode=quiet host_hostname )
	assertEquals "two" "${_test}"
}

. ${progdir}/../shunit2
