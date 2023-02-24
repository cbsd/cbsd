#!/bin/sh

# Scenario:
#  create exec1 jail
#  execute:
#
#    cbsd jexec jname=exec1

oneTimeSetUp(){
	jname=jcreate1
}

setUp() {
	dir=$(mktemp -d)
	cd "${dir}" || exit
}

tearDown() {
	cbsd jremove jname="${jname}"
	rm -rf "${dir}"
}

testFreeBSDVersion() {
	cbsd jcreate jname="${jname}" ver=13.2
	cbsd jstart jname="${jname}"
	jail_version=$(cbsd jexec jname="${jname}" freebsd-version | cut -d "-" -f 1-2 )		# trim -pXX (e.g.: 13.2-RELEASE-p11 -> 13.2-RELEASE )
	assertEquals "Jail FreeBSD version" "${jail_version}" "13.2-RELEASE"
}

# Test authorized_keys
testAuthorizedKeys() {
	cp ~cbsd/.ssh/authorized_keys "${dir}"/ || exit 1
	cbsd jcreate jname="${jname}" vnet=1 ip4_addr="212.212.212.214/30" ci_gw4="212.212.212.213" ci_user_pubkey="authorized_keys" runasap=1
	K1=$(head -n1 "${dir}"/authorized_keys)
	K2=$(head -n1 ~cbsd/jails-data/"${jname}"-data/root/.ssh/authorized_keys)
	assertNotNull "Empty orig authkey string" "${K1}"
	assertNotNull "Empty jail authkey string" "${K2}"
	assertSame "authorized_keys authkey string mismatch" "${K1}" "${K2}"
}

# check for sysrc
test_sysrc() {
	cbsd jcreate jname="${jname}" vnet=1 sysrc="ifconfig_eth0+='mtu 1450' inetd_enable=YES" runasap=1
	. ~cbsd/jails-data/"${jname}"-data/etc/rc.conf
	# get last world in ifconfig, should be 1450
	last=$(echo "${ifconfig_eth0}" | grep -o '[^ ]\+$')
	assertEquals "sysrc+= not valid" "${last}" "1450"
}

. shunit2
