#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

# get workdir
. /etc/rc.conf

jname="jcreate1"

ogw4="10.0.20.1"
ogw6="fdef:beb2:1929:8dba::1"
oipv4="10.0.20.2/24"			# wmask
onewipv4="10.0.20.3"			# NEW IPv4 without mask
oipv6="fdef:beb2:1929:8dba::2"		# without mask
oipv4_alias="192.168.0.2"		# without mask

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	set -o xtrace
	${CIX_BIN} jcreate runasap=0 vnet=1 ip4_addr=${oipv4},${oipv6},${oipv4_alias} ci_gw4=${ogw6},${ogw4} pkg_bootstrap=0 jname="${jname}"
	set +o xtrace
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

# just test if jstart works at all
test_jstart()
{
	${CIX_BIN} jstart jname="${jname}"
}

test_rcconf()
{
	unset defaultrouter
	unset ifconfig_eth0
	unset ifconfig_eth0_ipv6
	unset ifconfig_eth0_alias0
	unset ipv6_defaultrouter

	if [ ! -r ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf ]; then
		echo "no such ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf"
		exit 1
	fi

	cat ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf
	. ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf

	echo "check gw4"
	assertEquals "${defaultrouter}" "${ogw4}"
	echo "check gw6"
	assertEquals "${ipv6_defaultrouter}" "${ogw6}"
	echo "check IPv4-1"
	assertEquals "${ifconfig_eth0}" "inet ${oipv4}"
	echo "check IPv4 alias + mask"
	assertEquals "${ifconfig_eth0_alias0}" "inet ${oipv4_alias}/24"		# CBSD should append /24 for IPv4 without mask
	echo "check IPv6 + mask"
	assertEquals "${ifconfig_eth0_ipv6}" "inet6 ${oipv6}/64"			# CBSD should append /64 for IPv6 without mask
}

test_append_mtu()
{
	echo "append MTU"
	echo
	${CIX_DISTDIR}/misc/cbsdsysrc -qf ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf ifconfig_eth0+="mtu 9000"
	cat ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf
}

test_change_ipv4()
{
	${CIX_BIN} jstop jname="${jname}"
	${CIX_BIN} jset ip4_addr="${onewipv4},${oipv6},${oipv4_alias}" jname="${jname}"
	${CIX_BIN} jls
}

test_new_ipv4(){

	${CIX_BIN} jstart jname="${jname}"
	# now we must have 'ifconfig_eth0="inet <NEWIP>/24 mtu 9000'
	unset ifconfig_eth0
	cat ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf
	. ${cbsd_workdir}/jails-data/${jname}-data/etc/rc.conf
	echo "check IPv4-2"
	assertEquals "${ifconfig_eth0}" "inet ${onewipv4}/24 mtu 9000"
}

. ${progdir}/../shunit2
