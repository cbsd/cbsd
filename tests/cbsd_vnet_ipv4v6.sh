#!/bin/sh

jname="cbsd_test_vnetv4v6"

ogw4="10.0.20.1"
ogw6="fdef:beb2:1929:8dba::1"
oipv4="10.0.20.2/24"			# wmask
onewipv4="10.0.20.3"			# NEW IPv4 without mask
oipv6="fdef:beb2:1929:8dba::2"		# without mask
oipv4_alias="192.168.0.2"		# without mask


oneTimeSetUp() {
	if ! cbsd jstatus jname="${jname}" > /dev/null 2>&1; then
		echo "$0 destroy old ${jname}"
		cbsd jdestroy jname="${jname}"
	else
		set -o xtrace
		cbsd jcreate runasap=0 vnet=1 ip4_addr=${oipv4},${oipv6},${oipv4_alias} ci_gw4=${ogw6},${ogw4} pkg_bootstrap=0 jname="${jname}"
		set +o xtrace
	fi
}

setUp() {
	# nothing to do
}

tearDown() {
	# nothing to do
}

oneTimeTearDown() {
	cbsd jdestroy jname="${jname}"
}

# just test if jstart works at all
test_jstart(){
	cbsd jstart jname="${jname}"
}

test_rcconf(){

	unset defaultrouter
	unset ifconfig_eth0
	unset ifconfig_eth0_ipv6
	unset ifconfig_eth0_alias0
	unset ipv6_defaultrouter

	if [ ! -r ~cbsd/jails-data/${jname}-data/etc/rc.conf ]; then
		echo "no such ~cbsd/jails-data/${jname}-data/etc/rc.conf"
		exit 1
	fi

	cat ~cbsd/jails-data/${jname}-data/etc/rc.conf
	. ~cbsd/jails-data/${jname}-data/etc/rc.conf

	echo "check gw4"
	assertEquals "${defaultrouter}" "${ogw4}"
	echo "check gw6"
	assertEquals "${ipv6_defaultrouter}" "${ogw6}"
	echo "check IPv4"
	assertEquals "${ifconfig_eth0}" "inet ${oipv4}"
	echo "check IPv4 alias + mask"
	assertEquals "${ifconfig_eth0_alias0}" "inet ${oipv4_alias}/24"		# CBSD should append /24 for IPv4 without mask
	echo "check IPv6 + mask"
	assertEquals "${ifconfig_eth0_ipv6}" "inet6 ${oipv6}/64"			# CBSD should append /64 for IPv6 without mask
}

test_append_mtu(){
	sysrc -qf ~cbsd/jails-data/${jname}-data/etc/rc.conf ifconfig_eth0+="mtu 9000"
	cat ~cbsd/jails-data/${jname}-data/etc/rc.conf
}

test_change_ipv4(){
	cbsd jstop jname="${jname}"
	cbsd jset ip4_addr="${onewipv4},${oipv6},${oipv4_alias}" jname="${jname}"
	cbsd jls
}

test_new_ipv4(){
	cbsd jstart jname="${jname}"
	# now we must have 'ifconfig_eth0="inet <NEWIP>/24 mtu 9000'
	unset ifconfig_eth0
	cat ~cbsd/jails-data/${jname}-data/etc/rc.conf
	. ~cbsd/jails-data/${jname}-data/etc/rc.conf
	echo "check IPv4"
	assertEquals "${ifconfig_eth0}" "inet ${onewipv4}/24 mtu 9000"
}


. shunit2
