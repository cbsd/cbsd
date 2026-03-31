#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="jcreate1"

oneTimeTearDown()
{
	${CIX_BIN} jdestroy jname="${jname}"
}

oneTimeSetUp()
{
	${CIX_BIN} jcreate runasap=0 jname="${jname}" tags=tag1
}

test_tags1()
{
	_test=$( ${CIX_BIN} jls header=0 display=tags ${jname} )
	assertEquals "tag1" "${_test}"
}

test_tags2()
{
	${CIX_BIN} jset jname=${jname} tags=test_tag100,test_tag101,test_tag102
	_test=$( ${CIX_BIN} jls header=0 display=jname WHERE tags LIKE \'%test_tag101%\' )
	assertEquals "${jname}" "${_test}"
}

. ${progdir}/../shunit2
