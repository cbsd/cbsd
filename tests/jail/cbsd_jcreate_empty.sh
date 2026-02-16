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
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

testEmptyDataset()
{
	${CIX_BIN} jcreate jname="${jname}" baserw=1 ver=empty applytpl=0 mount_devfs=0 mount_ports=0 mount_src=0 floatresolv=0 etcupdate_init=0 pkg_bootstrap=0 quiet=1
	is_empty=$( ls ~cbsd/jails-data/${jname}-data/ | xargs )
	assertEquals "Check empty dataset" "${is_empty}" ""
}

. ${progdir}/../shunit2
