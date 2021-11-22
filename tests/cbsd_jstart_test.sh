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

# just test if jstart works at all
test_jstart(){
    cbsd jstart jname="${jname}"
    _test=$(jexec "${jname}" whoami)
    assertEquals "${_test}" "root"
}

# change hostname from inside jail
test_issue649_2() {
    cbsd jstart jname="${jname}"
    cbsd jexec jname="${jname}" sysrc hostname="lollipop"
    cbsd jstop jname="${jname}"
    cbsd jstart jname="${jname}"
    _test=$(cbsd jexec jname="${jname}" hostname)
    assertEquals "${_test}" "lollipop"
}

. shunit2
