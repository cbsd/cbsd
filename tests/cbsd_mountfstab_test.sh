#!/bin/sh

# Scenario:
#  create $jname jail
#  - modify jail.fstab for extra mount
#  execute:
#    cbsd mountfstab jname=$jname
#  check for additional mouns
# TODO : check for external script mount, e.g. fusefs-s3/ntfs..

jname="cbsd_test_fstabs"

oneTimeSetUp() {
	cbsd jdestroy jname="${jname}"
}

setUp() {
	cbsd jcreate runasap=0 jname="${jname}" pkg_bootstrap=0
}

tearDown() {
	cbsd jdestroy jname="${jname}"
}


test_create_extrafstab()
{
	cbsd jstart jname="${jname}"

	cat > ~cbsd/jails-fstab/${jname}/fstab.local <<EOF
/COPYRIGHT /var/TEST nullfs ro 0 0
EOF

	echo "extra fstab:"
	cat ~cbsd/jails-fstab/${jname}/fstab.local

	cbsd mountfstab ${jname}

	_test=$( jexec "${jname}" realpath /var/TEST )

	assertEquals "${_test}" "/var/TEST"
}

. shunit2

