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

oneTimeSetUp()
{
	jname="jcreate1"
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

tearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

testFreeBSDVersion()
{
	${CIX_BIN} jcreate jname="${jname}" ver=${DEFAULT_FREEBSD_JAIL_VER} etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=1
	jail_version=$( ${CIX_BIN} jexec jname="${jname}" freebsd-version | cut -d "-" -f 1-2 )		# trim -pXX (e.g.: 14.2-RELEASE-p11 -> 14.2-RELEASE )
	assertEquals "Jail FreeBSD version" "${jail_version}" "${DEFAULT_FREEBSD_JAIL_VER}-RELEASE"
}

# Test authorized_keys
testAuthorizedKeys()
{
	set -o xtrace
	${CIX_BIN} jcreate jname="${jname}" vnet=1 ip4_addr="212.212.212.214/30" ci_gw4="212.212.212.213" ci_user_pubkey="authorized_keys" runasap=1 interface=${DEFAULT_FREEBSD_JAIL_INTERFACE} pkg_bootstrap=0 etcupdate_init=0 quiet=1
	set +o xtrace
	K1=$( head -n1 ~cbsd/.ssh/id_rsa.pub )
	K2=$( head -n1 ~cbsd/jails-data/"${jname}"-data/root/.ssh/authorized_keys )
	assertNotNull "Empty orig authkey string" "${K1}"
	assertNotNull "Empty jail authkey string" "${K2}"
	assertSame "authorized_keys authkey string mismatch" "${K1}" "${K2}"
}

# Test for environments
testEnvironments()
{
	${CIX_BIN} jcreate jname="${jname}" environment="BOO1=foo1" environment="LOL5=foo5" pkg_bootstrap=0 runasap=1 etcupdate_init=0 quiet=1
	boo1_var=$( ${CIX_BIN} jexec jname="${jname}" env | grep BOO1= )
	lol5_var=$( ${CIX_BIN} jexec jname="${jname}" env | grep LOL5= )

	assertEquals "BOO1 var test" "${boo1_var}" "BOO1=foo1"
	assertEquals "LOL5 var test" "${lol5_var}" "LOL5=foo5"
}

# check for sysrc
test_sysrc()
{
	${CIX_BIN} jcreate jname="${jname}" vnet=1 ip4_addr="212.212.212.214/30" sysrc="\"ifconfig_eth0+='mtu 1450' p=1\"" runasap=1 interface=lo0 pkg_bootstrap=0 etcupdate_init=0 quiet=1
	. ~cbsd/jails-data/"${jname}"-data/etc/rc.conf

	# get last world in ifconfig, should be 1450
	last=$( echo "${ifconfig_eth0}" | grep -o '[^ ]\+$' )
	echo "last = [${last}]"
	assertEquals "sysrc+= not valid" "${last}" "1450"
	#assertSame "sysrc+= not valid" "${last}" "1450"
}

. ${progdir}/../shunit2
