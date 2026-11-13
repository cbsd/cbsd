#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="jrename1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate jname=${jname} ver=${DEFAULT_FREEBSD_JAIL_VER} etcupdate_init=0 pkg_bootstrap=0 quiet=1 runasap=0
}

oneTimeTearDown()
{
	for _jname in ${jname} jrename2 jrename3 jrename4; do
		${CIX_BIN} jstatus jname=${_jname} 2>/dev/null || ${CIX_BIN} jremove jname="${_jname}"
	done
}

# regression: jrename used double-quoted SQL string literals, which fail
# on SQLite built with SQLITE_DQS=0 ("no such column" swallowed by
# 2>/dev/null, misleading "No such jail" error)
test_jrename_offline()
{
	${CIX_BIN} jrename old=${jname} new=jrename2
	assertEquals "0" "$?"

	# note: 'jname' is not a jget JARG field, check via 'path' tail
	_test=$( ${CIX_BIN} jget jname=jrename2 mode=quiet path )
	assertEquals "jrename2" "${_test##*/}"

	# old jail must be gone: jget fails and prints error even in quiet mode
	${CIX_BIN} jget jname=${jname} mode=quiet path >/dev/null 2>&1
	assertNotEquals "0" "$?"
}

# rename to an existing name must fail
test_jrename_existing_target()
{
	${CIX_BIN} jrename old=jrename2 new=jrename2 2>/dev/null
	assertNotEquals "0" "$?"
}

# non-existing source must fail
test_jrename_no_such_jail()
{
	${CIX_BIN} jrename old=nonexistent_jail_xyz new=jrename2 2>/dev/null
	assertNotEquals "0" "$?"
}

# host_hostname must be updated during rename
test_jrename_host_hostname()
{
	${CIX_BIN} jrename old=jrename2 new=jrename3 host_hostname="jrename3.example.com"
	assertEquals "0" "$?"

	_test=$( ${CIX_BIN} jget jname=jrename3 mode=quiet host_hostname )
	assertEquals "jrename3.example.com" "${_test}"
}

# online jail: restart=1 must stop, rename and start again
test_jrename_online_restart()
{
	${CIX_BIN} jstart jname=jrename3

	${CIX_BIN} jrename old=jrename3 new=jrename4 restart=1
	assertEquals "0" "$?"

	_test=$( ${CIX_BIN} jget jname=jrename4 mode=quiet path )
	assertEquals "jrename4" "${_test##*/}"

	# jail must be online after restart=1: jstatus prints non-zero jid
	_jid=$( ${CIX_BIN} jstatus jname=jrename4 )
	assertNotEquals "0" "${_jid}"
}

. ${progdir}/../shunit2
