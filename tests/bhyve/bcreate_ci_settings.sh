#!/bin/sh
pgm="${0##*/}"			# Program basename
progdir="${0%/*}"		# Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${BHYVE_TEST_ENABLE}" != "1" ] && exit 0

# check for valid cloud-init yaml generate
jname="vmciset"
vm_os_type="freebsd"
vm_os_profile="${DEFAULT_FREEBSD_PROFILE}"

# flavor
ci_ip4_addr="10.0.1.88/22"
ci_gw4="10.0.1.1"
imgsize="10g"

oneTimeSetUp()
{
	if ! ${CIX_BIN} jstatus jname="${jname}" > /dev/null 2>&1; then
		echo "$0 destroy old ${jname}"
		${CIX_BIN} bdestroy jname="${jname}"
	else
		set -o xtrace
		${CIX_BIN} bcreate jname="${jname}" vm_ram=1g vm_cpus=1 vm_os_type=${vm_os_type} vm_os_profile=${vm_os_profile} imgsize=${imgsize} ci_ip4_addr=${ci_ip4_addr} ci_gw4=${ci_gw4}
		set +o xtrace
	fi
}

oneTimeTearDown()
{
	${CIX_BIN} bdestroy jname=${jname}
	return 0
}

setUp()
{
	# nothing to do
	true
}

tearDown()
{
	# nothing to do
	true
}

# todo - YAML/unmarshal/lint
test_ci_ip4_addr()
{
	local test=

	test=$( grep ${ci_ip4_addr} ~cbsd/jails-system/${jname}/cloud-init/network-config | awk '/address:/{print $2}' )
	echo "test val: ${test}"
	[ "${ci_ip4_addr}" != "${test}" ] && assertNull "ci_ip4_addr not equal: ${ci_ip4_addr} != ${test}" "${test}"

	return 0
}

test_ci_gw4()
{
	local test=

	# multiple gw?
	test=$( grep gateway: ~cbsd/jails-system/${jname}/cloud-init/network-config | awk '/gateway:/{print $2}' )
	echo "test val: ${test}"
	[ "${ci_gw4}" != "${test}" ] && assertNull "ci_gw4 not equal: ${ci_gw4} != ${test}" "${test}"

	return 0
}

. ${progdir}/../shunit2

