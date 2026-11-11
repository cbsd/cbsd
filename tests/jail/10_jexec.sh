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

jname="jexec1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} >/dev/null 2>&1 && ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate jname=${jname} host_hostname=${jname}.my.domain runasap=1 ver=${DEFAULT_FREEBSD_JAIL_VER} etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=1
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
	assertEquals "${jname}.my.domain" "${test}"
}

# pwd
testPwd()
{
	test="${CIX_BIN} jexec jname=${jname} dir=/tmp pwd"
	echo "test: ${test}"
	test=$(${test})
	assertEquals "/tmp" "${test}"
}

# environment= is repeatable and keeps spaces; the command is argv
testEnvironment()
{
	test=$(${CIX_BIN} jexec jname=${jname} environment="JEXEC_TEST_A=alpha" environment="JEXEC_TEST_B=beta space" -- /bin/sh -c 'printf %s "$JEXEC_TEST_A:$JEXEC_TEST_B"')
	assertEquals "alpha:beta space" "${test}"
}

# a name=value after -- is an argument, not an option
testDashDash()
{
	test=$(${CIX_BIN} jexec jname=${jname} -- printf '%s' user=nobody)
	assertEquals "user=nobody" "${test}"
}

# spaces and "=" stay inside one argument
testArgvLiteral()
{
	test=$(${CIX_BIN} jexec jname=${jname} -- printf '%s' 'a b=c')
	assertEquals "a b=c" "${test}"
}

# a newline inside one argument is not a separator
testArgvNewline()
{
	test=$(${CIX_BIN} jexec jname=${jname} -- printf '%s' 'line1
line2')
	assertEquals "line1
line2" "${test}"
}

# the program inside the jail reads this process's stdin
testStdin()
{
	test=$(printf 'hello-stdin' | ${CIX_BIN} jexec jname=${jname} -- cat)
	assertEquals "hello-stdin" "${test}"
}

# cmd= is a shell string when no program arguments are given
testCmdString()
{
	test=$(${CIX_BIN} jexec jname=${jname} cmd='printf %s cmd-ok')
	assertEquals "cmd-ok" "${test}"
}

# HEREDOC
testHereDoc()
{
	test=$(
		${CIX_BIN} jexec jname=${jname} <<EOF
hostname
EOF
	)
	assertEquals "${jname}.my.domain" "${test}"
}

### CBSDfile
testCBSDFile()
{
	cat >/tmp/test-CBSDfile <<________EOF
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
________EOF

	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} up cbsdfile=/tmp/test-CBSDfile

	. /etc/rc.conf

	assertTrue "[ -r ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file ]"
	test=$( cat ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file | awk '{printf $1}' )
	assertEquals "failed: not equal ${cbsd_workdir}/jails-data/${jname}-data/tmp/jexec.file:" "/tmp" "${test}"
	rm -f /tmp/test-CBSDfile
}

# TODO1: jexec jname='*'
# TODO2: CBSDfile + API

. ${progdir}/../shunit2
