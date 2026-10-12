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

	_vnet=$( ${CIX_BIN} jget jname="${jname}" mode=quiet vnet )
	assertEquals "vnet" "1" "${_vnet}"

	_iface=$( ${CIX_BIN} jget jname="${jname}" mode=quiet interface )
	assertEquals "interface" "${DEFAULT_FREEBSD_JAIL_INTERFACE}" "${_iface}"

	_ip=$( ${CIX_BIN} jget jname="${jname}" mode=quiet ip4_addr )
	assertEquals "ip4_addr" "192.168.20.117/24" "${_ip}"

	_gw=$( ${CIX_BIN} jget jname="${jname}" mode=quiet ci_gw4 )
	assertEquals "ci_gw4" "192.168.20.1" "${_gw}"

	_nic=$( ${CIX_BIN} jailnic jname="${jname}" mode=list header=0 display=nic_parent,nic_vlan_untagged | awk '{print $1, $2}' )
	assertEquals "jailnic" "${DEFAULT_FREEBSD_JAIL_INTERFACE} 20" "${_nic}"
}

. ${progdir}/../shunit2
