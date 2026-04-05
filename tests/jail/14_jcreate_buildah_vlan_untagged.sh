#!/bin/sh
# Scenario:
#  create empty jail
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
	/sbin/ifconfig ${DEFAULT_FREEBSD_JAIL_INTERFACE} vlanfilter
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

testLinuxVlanUntagged()
{
	${CIX_BIN} jcreate jname=${jname} \
		vnet=1 \
		interface=${DEFAULT_FREEBSD_JAIL_INTERFACE} \
		nic_vlan_untagged=20 \
		ip4_addr=192.168.20.117/24 \
		ci_gw4=192.168.20.1 \
		floatresolv=1 \
		platform=Linux \
		from=docker.io/library/busybox \
		exec_start="/bin/true" \
		runasap=1

	# TODO: how to test ?
	# ifconfig ${DEFAULT_FREEBSD_JAIL_INTERFACE} | grep 'untagged 20$'
	# e.g.:
#        member: epair2a flags=143<LEARNING,DISCOVER,AUTOEDGE,AUTOPTP>
#                port 10 priority 128 path cost 2000 vlan protocol 802.1q untagged 20

}

. ${progdir}/../shunit2
