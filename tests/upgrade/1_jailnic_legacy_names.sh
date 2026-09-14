#!/bin/sh

set -eu

test_root=$( mktemp -d )
trap 'rm -rf "${test_root}"' EXIT HUP INT TERM

mkdir -p "${test_root}/dist/subr" "${test_root}/dist/misc" \
	"${test_root}/db" "${test_root}/jails-system/legacy" \
	"${test_root}/jails-data/legacy-data/etc"

printf '%s\n' 'test_sql_stuff() { :; }' > "${test_root}/dist/subr/cbsdbootstrap.subr"
ln -s "$( command -v sqlite3 )" "${test_root}/dist/misc/sqlcli"

sqlite3 "${test_root}/db/local.real.sqlite" \
	"CREATE TABLE jails (jname TEXT, emulator TEXT, data TEXT); INSERT INTO jails VALUES ('legacy', 'jail', '${test_root}/jails-data/legacy-data');"
ln -s "${test_root}/db/local.real.sqlite" "${test_root}/db/local.sqlite"

jaildb="${test_root}/jails-system/legacy/local.sqlite"
sqlite3 "${jaildb}" "CREATE TABLE jailnic (name TEXT); INSERT INTO jailnic VALUES
	('eth0'), ('nic1'), ('nic2'), ('nic3'), ('wan'), ('nic3foo'), ('eth4'), ('nic4');"

# Primary, IPv6 and appended ifconfig_ethN assignments all confirm the
# historical interface name.  A commented assignment must not migrate nic3.
cat > "${test_root}/jails-data/legacy-data/etc/rc.conf" <<-EOF
	ifconfig_eth1="inet 192.0.2.1/24"
	ifconfig_eth2_ipv6="inet6 2001:db8::1/64"
	# ifconfig_eth3="inet 192.0.2.3/24"
	ifconfig_eth4+="mtu 1400"
	EOF

env CIX_DISTDIR="${test_root}/dist" \
	dbdir="${test_root}/db" \
	jailsysdir="${test_root}/jails-system" \
	jaildatadir="${test_root}/jails-data" \
	miscdir="${test_root}/dist/misc" \
	ECHO=echo GREP_CMD=grep N0_COLOR= N1_COLOR= N2_COLOR= \
	"${0%/*}/../../upgrade/pre-patch-15.0.9.1"

actual=$( sqlite3 "${jaildb}" "SELECT name FROM jailnic ORDER BY rowid;" )
expected='eth0
eth1
eth2
nic3
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
	jaildatadir="${test_root}/jails-data" \
	miscdir="${test_root}/dist/misc" \
	ECHO=echo GREP_CMD=grep N0_COLOR= N1_COLOR= N2_COLOR= \
	"${0%/*}/../../upgrade/pre-patch-15.0.9.1"

[ "$( sqlite3 "${jaildb}" "SELECT name FROM jailnic ORDER BY rowid;" )" = "${expected}" ]
