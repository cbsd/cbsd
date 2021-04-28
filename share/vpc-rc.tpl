#!/bin/sh
#
# PROVIDE: cbsd_vpc_%%VPC_NAME%%
# BEFORE: cbsdd
# REQUIRE: FILESYSTEMS NETWORKING ldconfig
# KEYWORD: shutdow
#
# cbsd_vpc_%%VPC_NAME%%_enable="YES"
#

. /etc/rc.subr

name=cbsd_vpc_%%VPC_NAME%%
rcvar=cbsd_vpc_%%VPC_NAME%%_enable
load_rc_config $name

: ${cbsd_vpc_%%VPC_NAME%%_enable="NO"}


start_cmd=${name}_start
stop_cmd=${name}_stop

command="/usr/bin/true"

cbsd_vpc_%%VPC_NAME%%_start()
{
	export PATH="/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin"
	cbsd vpc vpc_name=%%VPC_NAME%% mode=init_vxlan > /dev/null 2>&1
	cbsd vpc vpc_name=%%VPC_NAME%% mode=init_bridge bridge_ips="%%BRIDGE_IPS%%" /dev/null 2>&1
}

cbsd_vpc_%%VPC_NAME%%_stop()
{
	export PATH="/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin"
	/sbin/ifconfig %%MYBRIDGE%% destroy /dev/null 2>&1
}

run_rc_command "$1"
