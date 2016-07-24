#!/bin/sh

while getopts "c:l:d:" opt; do
	case "$opt" in
		c) conf="${OPTARG}" ;;
		l) orig_logfile="${OPTARG}" ;;
		d) debug="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

[ ! -f "${conf}" ] && exit 0
. ${conf}

[ -n "${orig_logfile}" ] && vm_logfile="${orig_logfile}"

detach=
[ "${2}" = "-d" ] && detach="-d"

[ -f /tmp/bhyvestop.${jname}.lock ] && /bin/rm -f /tmp/bhyvestop.${jname}.lock

. /etc/rc.conf
if [ -z "${cbsd_workdir}" ]; then
	echo "No cbsd workdir defined"
	exit 1
else
	workdir="${cbsd_workdir}"
fi

. ${workdir}/cbsd.conf

freebsdhostversion=$( ${miscdir}/elf_tables --ver /bin/sh )

while [ ! -f /tmp/bhyvestop.${jname}.lock  ]; do

	/usr/sbin/bhyvectl --vm=${jname} --destroy > /dev/null 2>&1
	/usr/bin/truncate -s0 ${vm_logfile}

	if [ ${cd_boot_once} -eq 0 ]; then
		if [ "${boot_from_grub}" = "1" ]; then
			echo "Booting from: ${vm_boot}" | /usr/bin/tee -a ${vm_logfile}
			# Bhyveload
			case "${vm_boot}" in
				"cd")
					echo "Boot from CD" | /usr/bin/tee -a ${vm_logfile}
					echo "DEBUG: $grub_iso_cmd" | /usr/bin/tee -a ${vm_logfile}
					eval "$grub_iso_cmd"
					;;
				"hdd")
					echo "Boot from HDD" | /usr/bin/tee -a ${vm_logfile}
					echo "DEBUG: ${grub_boot_cmd}" | /usr/bin/tee -a ${vm_logfile}
					eval "$grub_boot_cmd"
					;;
				*)
					echo "Booting from HDD" | /usr/bin/tee -a ${vm_logfile}
					eval "$grub_boot_cmd"
					;;
			esac
		else
			echo "DEBUG: $bhyveload_cmd" | /usr/bin/tee -a ${vm_logfile}
			eval "$bhyveload_cmd"
		fi
	else
		echo "Boot from CD" | /usr/bin/tee -a ${vm_logfile}
		if [ "${boot_from_grub}" = "1" ]; then
			echo "DEBUG: ${grub_iso_cmd}" | /usr/bin/tee -a ${vm_logfile}
			eval "${grub_iso_cmd}"
		else
			echo "DEBUG: ${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
			eval "${bhyveload_cmd}"
		fi
	fi

	case "${vm_boot}" in
		"cd")
			# add ,wait args when boot from CD
			if [ -n "${vnc_args}" -a "${vm_vnc_port}" != "1" ]; then
				orig_vnc_args="${vnc_args}"
				vnc_args="${vnc_args},wait"
			fi
			;;
		*)
	esac

	for i in ${mytap}; do
		/sbin/ifconfig ${i} up
	done

	if [ -n "${vm_vnc_port}" -a "${vm_vnc_port}" != "1" ]; then
		xhci_args="-s 30,xhci,tablet"
	else
		xhci_args=""
	fi

	#passthru
	echo "[debug] /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${lpc_args} ${efi_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} ${vnc_args} ${xhci_args} ${console_args} ${jname};"
	logger -t CBSD "[debug] /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${lpc_args} ${efi_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} ${vnc_args} ${xhci_args} ${console_args} ${jname};"

	# wait for VNC in upstream: xhci and vnc_args
	# /usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${lpc_args} ${efi_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} ${console_args} ${vnc_args} -s 30,xhci,tablet ${jname} >> ${vm_logfile} || touch /tmp/bhyvestop.${jname}.lock

#	echo "/usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${lpc_args} ${efi_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} ${console_args} ${jname}"
	/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} -A -H -P ${hostbridge_args} ${lpc_args} ${efi_args} ${virtiornd_args} ${nic_args} ${dsk_args} ${cd_args} ${console_args} ${vnc_args} ${xhci_args} ${jname} >> ${vm_logfile} || touch /tmp/bhyvestop.${jname}.lock

	# restore original value
	[ -n "${orig_vnc_args}" ] && vnc_args="${orig_vnc_args}"

	if [ ${cd_boot_once} -eq 1 ]; then
		# Eject cd
		cd_boot_once=0
		vm_boot="hdd"
		[ -n "${bhyveload_cmd_once}" ] && bhyveload_cmd="${bhyveload_cmd_once}"
		# replace hdd boot in conf
		/usr/sbin/sysrc -qf ${conf} cd_boot_once=0
		/usr/sbin/sysrc -qf ${conf} vm_boot=hdd
		/usr/sbin/sysrc -qf ${conf} bhyveload_cmd="${bhyveload_cmd}"
		# remove CD string for EFI
		/usr/sbin/sysrc -qf ${conf} cd_args=""
		unset cd_args
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
