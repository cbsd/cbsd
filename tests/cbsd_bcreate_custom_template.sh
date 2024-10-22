#!/bin/sh
# test custom profile location
profile_dir="/tmp/cbsd-profiles"
jname="custprofvm"
vm_os_type="freebsd"
vm_os_profile="cloud-FreeBSD-ufs-x64-14.1"
imgsize="1g"

oneTimeSetUp() {

	[ ! -d ${profile_dir} ] && mkdir -p ${profile_dir}

	cat > ${profile_dir}/vm-other-${jname}.conf <<EOF
vm_profile="${jname}"
vm_os_type="other"
long_description="Test custom profile"
fetch=0
iso_site=
iso_img=
iso_img_dist=
iso_extract=
register_iso_name=
register_iso_as=
default_jailname="${jname}"
xen_active=1
bhyve_active=1
qemu_active=1
vm_vnc_port="0"
vm_efi="uefi"
vm_package="small1"
sha256sum="0"
virtio_rnd="1"
EOF
}


oneTimeTearDown() {
	cbsd bdestroy jname=${jname}
	[ -d ${profile_dir} ] && rm -rf ${profile_dir}
}

setUp() {
	# nothing to do
}

tearDown() {
	# nothing to do
}

test_show_profile_list()
{
	local _test
	_test=$( env NOCOLOR=1 cbsd show_profile_list search_profile=vm-other show_bhyve=1 extra_profile_dir="${profile_dir}" display=path header=0 | while read _file; do
		if [ "${_file}" = "${profile_dir}/vm-other-${jname}.conf" ]; then
			echo "${profile_dir}/vm-other-${jname}.conf"
			return 0
		fi
	done )

	[ -n "${_test}" ] && return 0
	return 1
}

test_create_custom_vm()
{
	local _ret
	env NOCOLOR=1 cbsd bcreate jname=${jname} extra_profile_dir="${profile_dir}" vm_os_profile="${jname}" vm_os_type="other" imgsize=1g
	_ret=$?
	return ${_ret}
}

. shunit2
