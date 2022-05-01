#!/bin/sh

jname="cbsd_test_jstart"

oneTimeSetUp() {
	cbsd jdestroy jname="${jname}"
}

setUp() {
	cbsd jcreate runasap=0 jname="${jname}"
}

tearDown() {
	cbsd jdestroy jname="${jname}"
}

# https://github.com/cbsd/cbsd/issues/649
# change host_hostname
test_issue649_0() {
	cbsd jset jname="${jname}" host_hostname="one"
	_test=$( cbsd jget jname="${jname}" mode=quiet host_hostname )
	assertEquals "one" "${_test}"
}

# change host_hostname on-the-fly
test_issue649_1() {
	cbsd jstart jname="${jname}"
	cbsd jset jname="${jname}" host_hostname="two"
	cbsd jrestart jname="${jname}"
	_test=$( cbsd jget jname="${jname}" mode=quiet host_hostname )
	assertEquals "two" "${_test}"
}

. shunit2
