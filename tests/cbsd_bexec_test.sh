#!/bin/sh
# Scenario:
#  create exec1 jail
#  execute:
#
#    cbsd jexec jname=exec1

oneTimeSetUp() {
	jname="bexec1"
	cbsd bdestroy jname=${jname} || true
	workdir=$(mktemp -d)
	(
	cd "${workdir}" || exit
cat > CBSDfile << EOF
bhyve_${jname}()
{
	ci_ip4_addr="DHCP"
	ssh_wait=1
	runasap=1
	vm_os_type="linux"
	vm_os_profile="cloud-ubuntuserver-amd64-20.04"
	vm_ram="1g"
	vm_cpus="1"
	imgsize="10g"
}
EOF
	cbsd up
	)
	rm -rf "${workdir}"
}

oneTimeTearDown() {
	cbsd bdestroy jname="${jname}"
}

testBexec() {
	test=$(	cbsd bexec jname="${jname}" whoami )
	assertEquals "failed to run simple exec" "ubuntu" "${test}" 
}
testBexecCmd() {
	test=$(	cbsd bexec jname="${jname}" cmd=whoami )
	assertEquals "failed to run simple exec" "ubuntu" "${test}"
}

testBexecEOF() {
	test=$(	cbsd bexec jname="${jname}" cmd=whoami <<EOF
	whoami
EOF
)
	assertEquals "failed to run simple exec" "ubuntu" "${test}"
}
. shunit2
