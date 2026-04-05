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

jname="jcreate1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

#tearDown()
#{
#	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
#}

# Test authorized_keys
testAuthorizedKeys()
{
	. /etc/rc.conf

	set -o xtrace
	${CIX_BIN} jcreate jname="${jname}" vnet=1 ip4_addr="212.212.212.214/30" ci_gw4="212.212.212.213" ci_user_pubkey="authorized_keys" runasap=1 interface=${DEFAULT_FREEBSD_JAIL_INTERFACE} pkg_bootstrap=0 etcupdate_init=0 quiet=1
	set +o xtrace
#	K1=$( ${CIX_DISTDIR}/misc/cbsd_md5 ${cbsd_workdir}/.ssh/id_rsa.pub )
#	K2=$( ${CIX_DISTDIR}/misc/cbsd_md5 ${cbsd_workdir}/jails-data/"${jname}"-data/root/.ssh/authorized_keys )
	K1=$( head -n1 ${cbsd_workdir}/.ssh/id_rsa.pub )
	K2=$( head -n1 ${cbsd_workdir}/jails-data/"${jname}"-data/root/.ssh/authorized_keys )

	assertNotNull "Empty orig authkey string" "${K1}"
	assertNotNull "Empty jail authkey string" "${K2}"

	assertSame "authorized_keys authkey string mismatch" "${K1}" "${K2}"
}

. ${progdir}/../shunit2
