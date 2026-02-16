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
	jname="jexec1"
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate jname=${jname} runasap=1 ver=${DEFAULT_FREEBSD_JAIL_VER} etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=1
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
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
	cat >/tmp/test-CBSDfile <<EOF
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
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"

	${CIX_BIN} up cbsdfile=/tmp/test-CBSDfile

	. /etc/rc.conf

	assertTrue "[ -r ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file ]"
	test=$( cat ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file | awk '{printf $1}' )
	assertEquals "failed: not equal ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file:" "${test}" "/tmp"
	rm -f /tmp/test-CBSDfile
}

# TODO1: jexec jname='*'
# TODO2: CBSDfile + API

. ${progdir}/../shunit2
