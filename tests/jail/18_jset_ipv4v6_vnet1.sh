#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="cbsd_test_vnetv4v6"

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
	${CIX_BIN} jcreate runasap=1 vnet=1 ip4_addr=${oipv4} pkg_bootstrap=0 jname="${jname}"
	set +o xtrace
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

oneTimeTearDown()
{
	${CIX_BIN} jdestroy jname="${jname}"
}

test_setv4()
{
	unset defaultrouter
	unset ifconfig_eth0
	unset ifconfig_eth0_ipv6
	unset ifconfig_eth0_alias0
	unset ipv6_defaultrouter

	echo "set ip: ${onewipv4}"
	${CIX_BIN} jset jname="${jname}" ip4_addr="${onewipv4}"

	cat ~cbsd/jails-data/${jname}-data/etc/rc.conf
	. ~cbsd/jails-data/${jname}-data/etc/rc.conf

	echo "check IPv4 rc.conf"
	assertEquals "${ifconfig_eth0}" "inet ${onewipv4}/24"

	echo "check IPv4 ifconfig for old IPv4: ${oipv4}"
	${CIX_BIN} jexec jname="${jname}" /sbin/ifconfig | grep "inet ${oipv4} "
	ret=$?

	if [ ${ret} -eq 0 ]; then
		echo "error: old IP still exist: ${oipv4}"
		exit 1
	fi

	echo "check IPv4 ifconfig for new IPv4: ${onewipv4}/24"
	${CIX_BIN} jexec jname="${jname}" <<EOF
/sbin/ifconfig | grep "inet ${onewipv4} "
EOF
	ret=$?

	if [ ${ret} -ne 0 ]; then
		echo "error: new IP absent in ifconfig output: ${onewipv4}"
		exit 1
	fi
}

test_setv46()
{
	unset defaultrouter
	unset ifconfig_eth0
	unset ifconfig_eth0_ipv6
	unset ifconfig_eth0_alias0
	unset ipv6_defaultrouter

	echo "set ip: ${onewipv4},${oipv6}"
	${CIX_BIN} jset jname="${jname}" ip4_addr="${onewipv4},${oipv6}"

	cat ~cbsd/jails-data/${jname}-data/etc/rc.conf
	. ~cbsd/jails-data/${jname}-data/etc/rc.conf

	echo "check IPv4 rc.conf"
	assertEquals "${ifconfig_eth0}" "inet ${onewipv4}/24"

	echo "check IPv6 rc.conf"
	assertEquals "${ifconfig_eth0_ipv6}" "inet6 ${oipv6}/64"
}

. ${progdir}/../shunit2
