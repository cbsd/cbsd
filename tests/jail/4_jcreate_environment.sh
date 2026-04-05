#!/bin/sh
# Scenario:
#  create exec1 jail
#  execute:
#
#    ${CIX_BIN} jexec jname=exec1
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

# Test for environments
testEnvironments()
{
	${CIX_BIN} jcreate jname="${jname}" environment="BOO1=foo1" environment="LOL5=foo5" pkg_bootstrap=0 runasap=1 etcupdate_init=0 quiet=1
	boo1_var=$( ${CIX_BIN} jexec jname="${jname}" env | grep BOO1= )
	lol5_var=$( ${CIX_BIN} jexec jname="${jname}" env | grep LOL5= )

	assertEquals "BOO1 var test" "${boo1_var}" "BOO1=foo1"
	assertEquals "LOL5 var test" "${lol5_var}" "LOL5=foo5"
}

. ${progdir}/../shunit2
