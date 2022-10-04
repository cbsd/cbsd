#!/bin/sh
# check for valid cloud-init yaml generate
jname="vmciset"
vm_os_type="freebsd"
vm_os_profile="cloud-FreeBSD-ufs-x64-13.1"
ci_ip4_addr="10.0.1.88/22"
ci_gw4="10.0.1.1"
imgsize="10g"

oneTimeSetUp() {
	if ! cbsd jstatus jname="${jname}" > /dev/null 2>&1; then
		echo "$0 destroy old ${jname}"
		cbsd bdestroy jname="${jname}"
	else
		set -o xtrace
		cbsd bcreate jname="${jname}" vm_ram=1g vm_cpus=1 vm_os_type=${vm_os_type} vm_os_profile=${vm_os_profile} imgsize=${imgsize} ci_ip4_addr=${ci_ip4_addr} ci_gw4=${ci_gw4}
		set +o xtrace
	fi
}

oneTimeTearDown() {
	cbsd bdestroy jname=${jname}
}

setUp() {
	# nothing to do
}

tearDown() {
	# nothing to do
}

# todo - YAML/unmarshal/lint
test_ci_ip4_addr() {
	local test

	test=$( grep ${ci_ip4_addr} ~cbsd/jails-system/${jname}/cloud-init/network-config | awk '/address:/{print $2}' )
	echo "test val: ${test}"
	[ "${ci_ip4_addr}" != "${test}" ] && assertNull "ci_ip4_addr not equal: ${ci_ip4_addr} != ${test}" "${test}"
	return 0
}
test_ci_gw4() {
	local test

	# multiple gw?
	test=$( grep gateway: ~cbsd/jails-system/${jname}/cloud-init/network-config | awk '/gateway:/{print $2}' )
	echo "test val: ${test}"
	[ "${ci_gw4}" != "${test}" ] && assertNull "ci_gw4 not equal: ${ci_gw4} != ${test}" "${test}"
	return 0
}

. shunit2
