#!/bin/sh

jname="test"

oneTimeSetUp() {
	if cbsd jstatus jname="${jname}"; then
		cbsd jcreate jname="${jname}"
	fi
}

setUp() {
	cbsd expose jname=${jname} mode=flush
}

tearDown() {
	cbsd expose jname=${jname} mode=flush
}

test_expose_delete() {
	cbsd expose mode=add jname="${jname}" in=37684 out=37684
	test=$(cbsd expose mode=list jname="${jname}" | grep 37684)
	assertNotNull "Expose was not added" "${test}"

	cbsd expose mode=delete jname="${jname}" in=37684 out=37684
	test=$(cbsd expose mode=list jname="${jname}" | grep 37684)
	assertNull "Expose was not deleted" "${test}"
}

. shunit2
