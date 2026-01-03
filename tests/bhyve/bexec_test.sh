#!/bin/sh
# Scenario:
#  create exec1 jail
#  execute:
#
#    cbsd jexec jname=exec1
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${BHYVE_TEST_ENABLE}" != "1" ] && exit 0

oneTimeSetUp()
{
	jname="bexec1"
	${CIX_BIN} bdestroy jname=${jname} || true
	workdir=$( mktemp -d )
	(
		cd "${workdir}" || exit 1
		cat > CBSDfile <<________________EOF
bhyve_${jname}()
{
	ci_ip4_addr="DHCP"
	ssh_wait=1
	runasap=1
	vm_os_type="linux"
	vm_os_profile="${DEFAULT_LINUX_PROFILE}"
	vm_ram="1g"
	vm_cpus="1"
	imgsize="10g"
}
________________EOF
	${CIX_BIN} up
	)
	rm -rf "${workdir}"
}

oneTimeTearDown()
{
	${CIX_BIN} bdestroy jname="${jname}"
}

testBexec()
{
	test=$( ${CIX_BIN} bexec jname="${jname}" whoami )
	assertEquals "failed to run simple exec" "ubuntu" "${test}"
}

testBexecCmd()
{
	test=$( ${CIX_BIN} bexec jname="${jname}" cmd=whoami )
	assertEquals "failed to run simple exec" "ubuntu" "${test}"
}

testBexecEOF()
{
	test=$( ${CIX_BIN} bexec jname="${jname}" cmd=whoami <<________________EOF
whoami
________________EOF
	)
	assertEquals "failed to run simple exec" "ubuntu" "${test}"
}

. ${progdir}/../shunit2
