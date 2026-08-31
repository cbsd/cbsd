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

# check for sysrc
test_sysrc()
{
	set -o xtrace
	${CIX_BIN} jcreate jname="${jname}" vnet=1 ip4_addr="212.212.212.214/30" sysrc="\"ifconfig_eth0+='mtu 1450' p=1\"" runasap=1 pkg_bootstrap=0 etcupdate_init=0 quiet=1
	set +o xtrace
	. ~cbsd/jails-data/"${jname}"-data/etc/rc.conf
	# get last world in ifconfig, should be 1450
	last=$( echo "${ifconfig_eth0}" | grep -o '[^ ]\+$' )
	echo "last = [${last}]"
	assertEquals "sysrc+= not valid" "${last}" "1450"
	#assertSame "sysrc+= not valid" "${last}" "1450"
}

. ${progdir}/../shunit2
