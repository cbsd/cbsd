#!/bin/sh

# Scenario:
#  create empty jail

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
	cbsd jcreate jname="${jname}" baserw=1 ver=empty applytpl=0
	is_empty=$( ls -1 ~cbsd/jails-data/${jname}-data/ )
	assertEquals "Jail FreeBSD version" "${jail_version}" ""
}

. shunit2
