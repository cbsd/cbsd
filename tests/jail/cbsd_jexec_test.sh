#!/bin/sh
# Scenario:
#  create exec1 jail
#  execute:
#
#    ${CIX_BIN} jexec jname=exec1
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

oneTimeSetUp()
{
	ver="native"
	jname="jexec1"
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

setUp()
{
	${CIX_BIN} jcreate jname=${jname} runasap=1 ver="native"
	dir=$(mktemp -d)
	cd "${dir}" || exit
}

tearDown()
{
	${CIX_BIN} jremove ${jname}
	rm -rf "${dir}"
}

# simple jexec
testSimpleJexec()
{
	test="${CIX_BIN} jexec jname=${jname} hostname"
	echo "test: ${test}"
	test=$(${test})
	assertEquals "${test}" "${jname}.my.domain"
}

# pwd
testPwd()
{
	test="${CIX_BIN} jexec jname=${jname} dir=/tmp pwd"
	echo "test: ${test}"
	test=$(${test})
	assertEquals "${test}" "/tmp"
}

# HEREDOC
testHereDoc()
{
	test=$(
		${CIX_BIN} jexec jname=${jname} <<EOF
hostname
EOF
	)
	assertEquals "${test}" "${jname}.my.domain"
}

### CBSDfile
testCBSDFile()
{
	cat >CBSDfile <<EOF
quiet=0

jail_${jname}()
{
	ip4_addr="DHCP"
	host_hostname="${jname}.my.domain"
	pkg_bootstrap=0
	runasap=1
	ver="native"
}

postcreate_${jname}()
{
	set +o xtrace

	${CIX_DISTDIR}/misc/cbsdsysrc \
		syslogd_flags="-ss" \
		syslogd_enable="YES" \
		cron_enable="NO" \
		sendmail_enable="NO" \
		sendmail_submit_enable="NO"\
		sendmail_outbound_enable="NO" \
		sendmail_msp_queue_enable="NO" \
	# execute cmd inside jail
	jexec dir=/tmp <<XEOF
export PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin:/root/bin
pwd > /tmp/jexec.file
hostname
XEOF

}
EOF
	cp -a CBSDfile /tmp/

	${CIX_BIN} jdestroy jname=${jname} || true

	${CIX_BIN} up

	. /etc/rc.conf

	assertTrue "[ -r ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file ]"
	test=$( cat ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file | awk '{printf $1}' )
	assertEquals "failed: not equal ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file:" "${test}" "/tmp"
}

# TODO1: jexec jname='*'
# TODO2: CBSDfile + API

. ${progdir}/../shunit2
