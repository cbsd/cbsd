#!/bin/sh

conf="${1}"

[ ! -f "${conf}" ] && exit 0
. ${conf}

detach=
[ "${2}" = "-d" ] && detach="-d"

[ -f /tmp/bhyvestop.${jname}.lock ] && rm -f /tmp/bhyvestop.${jname}.lock

while [ ! -f /tmp/bhyvestop.${jname}.lock  ]; do

	/usr/sbin/bhyvectl --vm=${jname} --destroy > /dev/null 2>&1

	if [ ${cd_boot_once} -eq 0 ]; then
		if [ "${boot_from_grub}" = "1" ]; then
			echo "Booting from: ${vm_boot}"
			# Bhyveload
			case "${vm_boot}" in
				"cd")
					echo "Boot from CD"
					echo "DEBUG: $grub_iso_cmd"
					eval "$grub_iso_cmd"
					;;
				"hdd")
					echo "Boot from HDD"
					echo "DEBUG: ${grub_boot_cmd}"
					eval "$grub_boot_cmd"
					;;
				*)
					echo "Booting from HDD"
					eval "$grub_boot_cmd"
					;;
			esac
		else
			echo "DEBUG: $bhyveload_cmd"
			eval "$bhyveload_cmd"
		fi
	else
		echo "Boot from CD"
		if [ "${boot_from_grub}" = "1" ]; then
			echo "DEBUG: ${grub_iso_cmd}"
			eval "${grub_iso_cmd}"
		else
			echo "DEBUG: ${bhyveload_cmd}"
			eval "${bhyveload_cmd}"
		fi
	fi

	echo "[debug] /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${passthr} ${lpc_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} -l com1,stdio ${jname};"

	for i in ${mytap}; do
		/sbin/ifconfig ${i} up
	done
	/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${passthr} ${lpc_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} -l com1,stdio ${jname} || touch /tmp/bhyvestop.${jname}.lock
#	/usr/sbin/bhyvectl --get-vmcs-exit-reason --vm ${jname} >> /tmp/reason.txt
#	/usr/sbin/bhyvectl --get-vmcs-exit-ctls --vm ${jname} >> /tmp/reason.txt
#	/usr/sbin/bhyvectl --get-vmcs-exit-qualification --vm ${jname} >> /tmp/reason.txt
#	/usr/sbin/bhyvectl --get-vmcs-exit-interruption-info --vm ${jname} >> /tmp/reason.txt
#	/usr/sbin/bhyvectl --get-vmcs-exit-interruption-error --vm ${jname} >> /tmp/reason.txt
	if [ ${cd_boot_once} -eq 1 ]; then
		# Eject cd
		cd_boot_once=0
		vm_boot="hdd"
		[ -n "${bhyveload_cmd_once}" ] && bhyveload_cmd="${bhyveload_cmd_once}"
		# replace hdd boot in conf
		/usr/sbin/sysrc -qf ${conf} cd_boot_once=0
		/usr/sbin/sysrc -qf ${conf} vm_boot=hdd
		/usr/sbin/sysrc -qf ${conf} bhyveload_cmd="${bhyveload_cmd}"
	fi
	reset
	clear
done

/bin/rm -f /tmp/bhyvestop.${jname}.lock
/usr/local/bin/cbsd bstop ${jname}
for i in ${mytap}; do
	/sbin/ifconfig ${i} destroy
done
exit ${bhyve_exit}
