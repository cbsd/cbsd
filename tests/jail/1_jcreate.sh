#!/bin/sh
# Scenario:
#  simplle jcreate

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
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

#tearDown()
#{
#	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
#}

testFreeBSDVersion()
{
	local tmpver=$( uname -r )
	local ver=${tmpver%%-*}

	${CIX_BIN} jcreate jname="${jname}" ver=native etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=1
	jail_version=$( ${CIX_BIN} jexec jname="${jname}" freebsd-version | cut -d "-" -f 1-2 )		# trim -pXX (e.g.: 14.2-RELEASE-p11 -> 14.2-RELEASE )
	assertEquals "Jail FreeBSD version" "${jail_version}" "${ver}-RELEASE"
}

. ${progdir}/../shunit2
