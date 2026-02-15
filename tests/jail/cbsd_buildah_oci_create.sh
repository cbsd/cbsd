#!/bin/sh
# Scenario:
#  create empty jail
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname=jcreate1

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

testLinuxOS()
{
	${CIX_BIN} jcreate jname="${jname}" runasap=1 from=docker.io/library/busybox platform=linux emulator=linux pkg_bootstrap=0 floatresolv=0 applytpl=0 etcupdate_init=0
	is_linux=$( ${CIX_BIN} jexec jname="${jname}" uname -s )
	assertEquals "Linux OS" "${is_linux}" "Linux"
}

. ${progdir}/../shunit2
