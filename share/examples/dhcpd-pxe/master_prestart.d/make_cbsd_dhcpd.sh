#!/usr/local/bin/cbsd
# Helper for integration CBSD/bhyve and isc-dhcpd
# We assume bhyve have correct 'ip4_addr' settings
# To add dhcpf.conf extra-config (e.g. bootp-related) for
#    hosts ${jname} {  ..  }
#    please use file in ${jailsysdir}/${jname}/dhcpd_extra.conf
#    << all content from this file will be dropped to { } block
#
# Additional: https://www.bsdstore.ru/en/articles/cbsd_vm_hook_dhcpd.html
#
# Autor: olevole@olevole.ru

DHCPD_CONF="/root/etc/dhcpd.conf"

#. /etc/rc.conf

#workdir="${cbsd_workdir}"

set -e
. ${workdir}/cbsd.conf
. ${subr}
. ${cbsdinit}
set +e

export NOCOLOR=1

[ -z "${jname}" ] && err 1 "no jname variable"
[ -z "${nic_hwaddr0}" ] && err 1 "no nic_hwaddr0 variable"
[ -z "${ip4_addr}" ] && err 1 "no ip4_addr variable"
[ ! -r "${DHCPD_CONF}" ] && err 1 "no ${DHCPD_CONF}"

EXTRA_CONF="${jailsysdir}/${jname}/dhcpd_extra.conf"

case "${ip4_addr}" in
	[Dd][Hh][Cc][Pp])
		ip4_addr=$( dhcpd )
		;;
	*)
		;;
esac

ipwmask "${ip4_addr}"
[ -n "${IWM}" ] && ip="${IWM}"
iptype ${ip}

ret=$?

case "${ret}" in
	0)
		echo "IPv4 detected"
		;;
	1)
		echo "IPv6 detected"
		;;
	2)
		# Fake DHCP - we need learn to get IPS from real dhcpd (tap iface + mac ?)
		tmp_addr=$( dhcpd )
		ip=${tmp_addr%%/*}
		# check again
		ip_type ${ip}
		ret=$?
		case "${ret}" in
			0|1)
				# update new IP
				bset ip4_addr="${ip}" jname="${jname}"
				;;
			*)
				err 0 "Can't obtain DHCP addr"
				;;
		esac
		;;
	*)
		err 0 "Unknown IP type: ${ip}"
		;;
esac

# Remove old records for this host if exist
if /usr/bin/grep "CBSD-AUTO-${jname}" ${DHCPD_CONF} >/dev/null 2>&1; then
	/bin/cp -a ${DHCPD_CONF} /tmp/dhcpd.tmp.$$
	trap "/bin/rm -f /tmp/dhcpd.tmp.$$" HUP INT ABRT BUS TERM EXIT
	/usr/bin/grep -v "CBSD-AUTO-${jname}" /tmp/dhcpd.tmp.$$ > ${DHCPD_CONF}
fi

# Insert new records into config file
/bin/cat >> ${DHCPD_CONF} <<EOF
host ${jname} {					# CBSD-AUTO-${jname}
	hardware ethernet ${nic_hwaddr0};	# CBSD-AUTO-${jname}
	fixed-address ${ip};			# CBSD-AUTO-${jname}
EOF

if [ -r "${EXTRA_CONF}" ]; then
	echo "Found extra conf: ${EXTRA_CONF}"
	/bin/cat ${EXTRA_CONF} |while read _line; do
		/bin/cat >> ${DHCPD_CONF} <<EOF
	${_line}				# CBSD-AUTO-${jname}
EOF
	done
fi

/bin/cat >> ${DHCPD_CONF} <<EOF
}				# CBSD-AUTO-${jname}
EOF

/usr/sbin/arp -d ${ip}
/usr/sbin/arp -s ${ip} ${nic_hwaddr0} pub

#service isc-dhcpd restart
/usr/sbin/service isc-dhcpd stop
/usr/sbin/service isc-dhcpd start
