#!/bin/sh

set -eu

test_root=$( mktemp -d )
trap 'rm -rf "${test_root}"' EXIT HUP INT TERM

mkdir -p "${test_root}/dist/subr" "${test_root}/dist/misc" \
	"${test_root}/db" "${test_root}/jails-system/legacy"

printf '%s\n' 'test_sql_stuff() { :; }' > "${test_root}/dist/subr/cbsdbootstrap.subr"
ln -s "$( command -v sqlite3 )" "${test_root}/dist/misc/sqlcli"

sqlite3 "${test_root}/db/local.real.sqlite" \
	"CREATE TABLE jails (jname TEXT, emulator TEXT); INSERT INTO jails VALUES ('legacy', 'jail');"
ln -s "${test_root}/db/local.real.sqlite" "${test_root}/db/local.sqlite"

jaildb="${test_root}/jails-system/legacy/local.sqlite"
sqlite3 "${jaildb}" "CREATE TABLE jailnic (name TEXT); INSERT INTO jailnic VALUES
	('eth0'), ('nic1'), ('nic2'), ('wan'), ('nic3foo'), ('eth4'), ('nic4');"

env CIX_DISTDIR="${test_root}/dist" \
	dbdir="${test_root}/db" \
	jailsysdir="${test_root}/jails-system" \
	miscdir="${test_root}/dist/misc" \
	ECHO=echo N0_COLOR= N1_COLOR= N2_COLOR= \
	"${0%/*}/../../upgrade/pre-patch-15.0.9.1"

actual=$( sqlite3 "${jaildb}" "SELECT name FROM jailnic ORDER BY rowid;" )
expected='eth0
eth1
eth2
wan
nic3foo
eth4
nic4'

if [ "${actual}" != "${expected}" ]; then
	printf 'Unexpected jailnic names:\n%s\n' "${actual}" >&2
	exit 1
fi

# The migration must be safe to run during every initenv invocation.
env CIX_DISTDIR="${test_root}/dist" \
	dbdir="${test_root}/db" \
	jailsysdir="${test_root}/jails-system" \
	miscdir="${test_root}/dist/misc" \
	ECHO=echo N0_COLOR= N1_COLOR= N2_COLOR= \
	"${0%/*}/../../upgrade/pre-patch-15.0.9.1"

[ "$( sqlite3 "${jaildb}" "SELECT name FROM jailnic ORDER BY rowid;" )" = "${expected}" ]

