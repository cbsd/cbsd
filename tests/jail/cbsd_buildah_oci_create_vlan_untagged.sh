#!/bin/sh
# Scenario:
#  create empty jail
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

}

. ${progdir}/../shunit2
