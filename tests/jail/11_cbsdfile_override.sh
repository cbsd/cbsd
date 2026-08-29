#!/bin/sh
# Scenario:
#  cbsd up param1=var1 param2=var2
#  execute:
#
#    ${CIX_BIN} jexec jname=exec1
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="jexec1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate jname=${jname} runasap=1 ver=${DEFAULT_FREEBSD_JAIL_VER} etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=1
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

# simple jexec
testSimpleJexec()
{
	test="${CIX_BIN} jexec jname=${jname} hostname"
	echo "test: ${test}"
	test=$(${test})
	assertEquals "${test}" "${jname}.my.domain"
}

### CBSDfile
testCBSDFile()
{
	cat >/tmp/test-CBSDfile <<________EOF
quiet=0

jail_${jname}()
{
	ip4_addr="10.10.10.10"
	host_hostname="${jname}.my.domain"
	pkg_bootstrap=0
	runasap=0
	ver="15.0"
}
________EOF
 
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} up cbsdfile=/tmp/test-CBSDfile
#	rm -f /tmp/test-CBSDfile
}

. ${progdir}/../shunit2
