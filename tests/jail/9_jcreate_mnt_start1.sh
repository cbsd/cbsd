#!/bin/sh
# Scenario:
#  create jail from snap
pgm="${0##*/}"			# Program basename
progdir="${0%/*}"		# Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="mnttest1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname="${jname}" 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"

	#  ${mnt_start} -d ${data} -f ${jailfstabdir}/${jname} -j ${jname} -r ${jailrcconfdir} -s ${jailsysdir}/${jname}

	[ -r /tmp/cbsd_jcreate_test_mnt_start1.sh ] && rm -f /tmp/cbsd_jcreate_test_mnt_start1.sh

	cat > /tmp/cbsd_jcreate_test_mnt_start1.sh<<________EOF
#!/bin/sh

while getopts "j:d:f:r:s:" opt; do
	case "\${opt}" in
		d) data="\${OPTARG}" ;;
		j) jname="\${OPTARG}" ;;
	esac
	shift \$((\$OPTIND - 1))
done

[ -d "/tmp/cbsd_jcreate_test_mnt_start1-data" ] && rm -rf "/tmp/cbsd_jcreate_test_mnt_start1-data"
mkdir /tmp/cbsd_jcreate_test_mnt_start1-data
[ ! -d "\${data}" ] && mkdir -p \${data}
echo "mount -t nullfs /tmp/cbsd_jcreate_test_mnt_start1-data \${data}"
mount -t nullfs /tmp/cbsd_jcreate_test_mnt_start1-data \${data} || true
________EOF

	chmod +x /tmp/cbsd_jcreate_test_mnt_start1.sh
}

oneTimeTearDown()
{
	local _ret=

	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	[ -d /tmp/cbsd_jcreate_test_mnt_start1-data ] && rmdir /tmp/cbsd_jcreate_test_mnt_start1-data
	[ -r /tmp/cbsd_jcreate_test_mnt_start1.sh ] && rm -f /tmp/cbsd_jcreate_test_mnt_start1.sh
	return 0
}

testCreateJail()
{
	local _ret=
	${CIX_BIN} jcreate jname="${jname}" mnt_start=/tmp/cbsd_jcreate_test_mnt_start1.sh

	test -r /tmp/cbsd_jcreate_test_mnt_start1-data/etc/rc.conf
	_ret=$?
	assertEquals "no such /etc/rc.conf file in source mnt_start dir: /tmp/cbsd_jcreate_test_mnt_start1-data" 0 "${_ret}"
}

. ${progdir}/../shunit2
