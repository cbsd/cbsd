#!/bin/sh
# test custom profile location
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${BHYVE_TEST_ENABLE}" != "1" ] && exit 0

profile_dir="/tmp/cbsd-profiles"
jname="custprofvm"
vm_os_type="freebsd"
vm_os_profile="cloud-FreeBSD-15.1-ufs-x86_64"
imgsize="1g"

oneTimeSetUp()
{
	[ ! -d ${profile_dir} ] && mkdir -p ${profile_dir}

	cat > ${profile_dir}/vm-other-${jname}.conf <<________EOF
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
________EOF

}

oneTimeTearDown()
{
	${CIX_BIN} bdestroy jname=${jname}
	[ -d "${profile_dir}" ] && rm -rf "${profile_dir}"

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

test_show_profile_list()
{
	local _test=

	_test=$( ${CIX_BIN} show_profile_list search_profile=vm-other show_bhyve=1 extra_profile_dir="${profile_dir}" display=path header=0 | while read _file; do
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
	local _ret=

	${CIX_BIN} bcreate jname=${jname} extra_profile_dir="${profile_dir}" vm_os_profile="${jname}" vm_os_type="other" imgsize=1g
	_ret=$?
	return ${_ret}
}

. ${progdir}/../shunit2
