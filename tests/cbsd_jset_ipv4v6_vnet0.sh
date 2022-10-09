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
		cbsd jcreate runasap=1 vnet=0 ip4_addr=${oipv4} pkg_bootstrap=0 jname="${jname}"
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

test_setv4(){

	unset defaultrouter
	unset ifconfig_eth0
	unset ifconfig_eth0_ipv6
	unset ifconfig_eth0_alias0
	unset ipv6_defaultrouter

	echo "set ip: ${onewipv4}"
	cbsd jset jname="${jname}" ip4_addr="${onewipv4}"
	arp -ad

	/sbin/ifconfig |grep "inet ${oipv4} "
	ret=$?

	if [ ${ret} -eq 0 ]; then
		echo "error: old IP still exist: ${oipv4}"
		exit 1
	fi

	/sbin/ifconfig |grep "inet ${onewipv4} "
	ret=$?

	if [ ${ret} -ne 0 ]; then
		echo "error: new IP absent in ifconfig output: ${onewipv4}"
		exit 1
	fi
}

test_setv46(){

	unset defaultrouter
	unset ifconfig_eth0
	unset ifconfig_eth0_ipv6
	unset ifconfig_eth0_alias0
	unset ipv6_defaultrouter

	echo "set ip: ${onewipv4},${oipv6}"
	cbsd jset jname="${jname}" ip4_addr="${onewipv4},${oipv6}"
	arp -ad

	/sbin/ifconfig |grep "inet ${onewipv4} "
	ret=$?

	if [ ${ret} -ne 0 ]; then
		echo "error: new IP absent in ifconfig output: ${onewipv4}"
		exit 1
	fi

	/sbin/ifconfig |grep "inet6 ${oipv6} "
	ret=$?

	if [ ${ret} -ne 0 ]; then
		echo "error: new IP absent in ifconfig output: ${oipv6}"
		exit 1
	fi
}

. shunit2
