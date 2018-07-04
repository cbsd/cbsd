#!/bin/sh

while getopts "c:d:l:r:" opt; do
	case "$opt" in
		c) conf="${OPTARG}" ;;
		d) debug="${OPTARG}" ;;
		l) orig_logfile="${OPTARG}" ;;
		r) restore_checkpoint="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

[ ! -f "${conf}" ] && exit 0
. ${conf}

[ -n "${orig_logfile}" ] && vm_logfile="${orig_logfile}"

if [ -n "${restore_checkpoint}" ]; then
	if [ ! -r ${restore_checkpoint} ]; then
		echo "No checkpoint here: ${restore_checkpoint}"
		exit 1
	fi
fi

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
		echo "DEBUG: ${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
		eval "${bhyveload_cmd}"
	else
		echo "Boot from CD" | /usr/bin/tee -a ${vm_logfile}
		echo "DEBUG: ${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
		eval "${bhyveload_cmd}"
	fi

	case "${vm_boot}" in
		"cd")
			# add ,wait args when boot from CD
			if [ "${vm_efi}" != "none" ]; then
				if [ -n "${vnc_args}" -a "${vm_vnc_port}" != "1" ]; then
					orig_vnc_args="${vnc_args}"
					# bhyve_vnc_vgaconf before wait
					if [ "${bhyve_vnc_vgaconf}" != "io" ]; then
						[ -n "${bhyve_vnc_vgaconf}" ] && vnc_args="${vnc_args},vga=${bhyve_vnc_vgaconf}"
					fi
					orig_vnc_args="${vnc_args}"
					if [ "${cd_vnc_wait}" = "1" ]; then
						echo "Waiting for first connection via VNC to starting VMs..."
						vnc_args="${vnc_args},wait"
					fi
				fi
			fi
			;;
		*)
			# bhyve_vnc_vgaconf before wait
			if [ "${bhyve_vnc_vgaconf}" != "io" ]; then
				[ -n "${bhyve_vnc_vgaconf}" ] && vnc_args="${vnc_args},vga=${bhyve_vnc_vgaconf}"
			fi
	esac

	for i in ${mytap}; do
		/sbin/ifconfig ${i} up
	done

	[ ${freebsdhostversion} -lt 1100120 ] && vm_vnc_port=1 # Disable xhci on FreeBSD < 11

	if [ "${vm_efi}" != "none" ]; then
		if [ -n "${vm_vnc_port}" -a "${vm_vnc_port}" != "1" ]; then
			xhci_args="-s 30,xhci,tablet"

			# VNC password support introduced in FreeBSD 11.1+
			if [ ${freebsdhostversion} -gt 1101500 ]; then
				if [ -n "${vnc_password}" ]; then
					vnc_args="${vnc_args},password=${vnc_password}"
				fi
			fi

		else
			xhci_args=""
		fi
	fi

	add_bhyve_opts="-H"  # Yield the virtual CPU thread when a HLT instruction is detected.

	[ "${bhyve_generate_acpi}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -A"

	[ "${bhyve_wire_memory}" = "1" -o -n "${pci_passthru_args}" ] && add_bhyve_opts="${add_bhyve_opts} -S"

	[ "${bhyve_rts_keeps_utc}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -u"
	[ "${bhyve_force_msi_irq}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -W"
	[ "${bhyve_x2apic_mode}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -x"
	[ "${bhyve_mptable_gen}" = "0" ] && add_bhyve_opts="${add_bhyve_opts} -Y" # disable mptable gen
	[ "${bhyve_ignore_msr_acc}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -w"

	checkpoint_args=

	[ -n "${restore_checkpoint}" ] && checkpoint_args="-r ${restore_checkpoint}"

	bhyve_cmd="/usr/bin/nice -n ${nice} /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} ${add_bhyve_opts} ${hostbridge_args} ${virtio_9p_args} ${uefi_boot_args} ${dsk_args} ${dsk_controller_args} ${cd_args} ${nic_args} ${virtiornd_args} ${pci_passthru_args} ${vnc_args} ${xhci_args} ${lpc_args} ${console_args} ${efi_args} ${checkpoint_args} ${jname}"

	echo "[debug] ${bhyve_cmd}"
	logger -t CBSD "[debug] ${bhyve_cmd}"
	echo "cmd: ${bhyve_cmd}" >> ${vm_logfile}
	echo "-----" >>  ${vm_logfile}

	/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock ${bhyve_cmd} >> ${vm_logfile} 2>&1
	ret=$?
	if [ ${ret} -ne 0 ]; then
		touch /tmp/bhyvestop.${jname}.lock
		echo "Exit code: ${ret}. See ${vm_logfile} for details"
		echo
		tail -n50 ${vm_logfile}
		echo "Sleep 15 seconds..."
		sleep 15
	else
		/bin/rm -f ${vm_logfile}
	fi

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
		if [ "${vm_efi}" != "none" ]; then
			if [ -n "${cd_args2}" ]; then
				/usr/sbin/sysrc -qf ${conf} cd_args="${cd_args2}"
				cd_args="${cd_args2}"
			else
				/usr/sbin/sysrc -qf ${conf} cd_args=""
				unset cd_args
			fi
		else
			/usr/sbin/sysrc -qf ${conf} cd_args=""
			unset cd_args
		fi
	fi
	reset
	clear
done

# extra destroy
/usr/bin/nice -n ${nice} /usr/sbin/bhyvectl --vm=${jname} --destroy > /dev/null 2>&1 || true
/bin/rm -f /tmp/bhyvestop.${jname}.lock
# extra stop
/usr/local/bin/cbsd bstop cbsd_queue_name=none jname=${jname}

for i in ${mytap}; do
	/sbin/ifconfig ${i} destroy
done

exit ${bhyve_exit}
