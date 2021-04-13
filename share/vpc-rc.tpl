#!/bin/sh
#
# PROVIDE: cbsd_vpc_%%VPC_NAME%%
# REQUIRE: cbsdd
# KEYWORD: shutdown
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
	cbsd vpc vpc_name=%%VPC_NAME%% mode=init_vxlan
	cbsd vpc vpc_name=%%VPC_NAME%% mode=init_bridge bridge_ips="%%BRIDGE_IPS%%"
}

cbsd_vpc_%%VPC_NAME%%_stop()
{
	export PATH="/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin"
	/sbin/ifconfig %%MYBRIDGE%% destroy
}

run_rc_command "$1"
