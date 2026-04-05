#!/bin/sh
pgm="${0##*/}"                  # Program basename
progdir="${0%/*}"               # Program directory

set -e
. ${progdir}/../config.conf
set +e

[ "${JAIL_TEST_ENABLE}" != "1" ] && exit 0

jname="jcreate1"

oneTimeSetUp()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
	${CIX_BIN} jcreate runasap=0 jname="${jname}"
}

oneTimeTearDown()
{
	${CIX_BIN} jstatus jname=${jname} 2>/dev/null || ${CIX_BIN} jremove jname="${jname}"
}

#tearDown()
#{
#	${CIX_BIN} jdestroy jname="${jname}"
#}

test_facts_0()
{
	. /etc/rc.conf
	cat > ${cbsd_workdir}/jails-system/${jname}/facts.d/x <<________EOF
#!/bin/sh
echo "FOO"
________EOF

	chmod +x ${cbsd_workdir}/jails-system/${jname}/facts.d/x
	_test=$( ${CIX_BIN} jls header=0 display=x ${jname} )
	assertEquals "FOO" "${_test}"
}

. ${progdir}/../shunit2
