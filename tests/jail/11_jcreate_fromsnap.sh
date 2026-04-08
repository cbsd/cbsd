#!/bin/sh
# Scenario:
#  create jail from snap
pgm="${0##*/}"			# Program basename
progdir="${0%/*}"		# Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

# hardcoded
#CBSD_JAIL_ROOT="zroot/jails"
CBSD_JAIL_ROOT="tank/jails"

zfs list ${CBSD_JAIL_ROOT} > /dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "no such ZFS: ${CBSD_JAIL_ROOT}, SKIPP"
	exit 0
fi

gold_jname="goldtestjail1"
jname="goldclone1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${gold_jname} 2>/dev/null || ${CIX_BIN} jremove jname="${gold_jname}"
}

oneTimeTearDown()
{
	local _ret=
	${CIX_BIN} jstatus jname=${gold_jname} 2>/dev/null || ${CIX_BIN} jremove jname="${gold_jname}"
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"

	zfs list -t snapshot ${CBSD_JAIL_ROOT}/${gold_jname}@init 2>/dev/null
	_ret=$?
	if [ ${_ret} -eq 0 ]; then
		zfs destroy -r ${CBSD_JAIL_ROOT}/${gold_jname}@init
		zfs destroy ${CBSD_JAIL_ROOT}/${gold_jname}
	fi
}

testCreateGold1()
{
	local _ret=
	${CIX_BIN} jcreate jname="${gold_jname}" ver=native etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=1

	# create artifacts
	${CIX_BIN} jexec jname="${gold_jname}" cmd="date > /root/date.txt"
	${CIX_BIN} jexec jname="${gold_jname}" test -r /root/date.txt
	_ret=$?

	# test: 0 - ok
	assertEquals "File /root/date.txt not found inside Gold jail" 0 "${_ret}"

	GOLD_MNTPT=$( zfs get -Ho value mountpoint ${CBSD_JAIL_ROOT}/${gold_jname} 2>/dev/null )
	_ret=$?

	# test: 0 - ok
	assertEquals "found mountpoint for gold jail not found: zfs get -Ho value mountpoint ${CBSD_JAIL_ROOT}/${gold_jname}: " 0 "${_ret}"

	zfs snapshot ${CBSD_JAIL_ROOT}/${gold_jname}@init
	_ret=$?

	# test: 0 - ok
	assertEquals "zfs snapshot failed for gold jail: zfs snapshot ${CBSD_JAIL_ROOT}/${gold_jname}@init: " 0 "${_ret}"
}

testCreateClone1()
{
	local _ret=

	${CIX_BIN} jcreate jname="${jname}" ver=native etcupdate_init=0 pkg_bootstrap=0 quiet=1 zfs_snapsrc=${CBSD_JAIL_ROOT}/${gold_jname}@init runasap=1
	_ret=$?

	# test: 0 - ok
	assertEquals "${CIX_BIN} jcreate jname="${jname}" ver=native etcupdate_init=0 pkg_bootstrap=0 quiet=1 zfs_snapsrc=${CBSD_JAIL_ROOT}/${gold_jname}@init runasap=1" 0 "${_ret}"

	${CIX_BIN} jexec jname="${jname}" test -r /root/date.txt
	_ret=$?

	# test: 0 - ok
	assertEquals "File /root/date.txt not found inside Cloned jail" 0 "${_ret}"
}

. ${progdir}/../shunit2
