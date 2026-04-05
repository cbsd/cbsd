#!/bin/sh
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
	${CIX_BIN} jcreate jname=${jname} runasap=1 ver=${DEFAULT_FREEBSD_JAIL_VER} etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=0
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
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
