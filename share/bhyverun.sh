#!/bin/sh

on_crash_status()
{
	local _ret=0

	case "${on_crash}" in
		restart)
			_ret=1
			;;
		destroy)
			_ret=0
			;;
		*)
			logger -t bhyverun.sh "unknown value for on_poweroff: ${on_poweroff}"
			_ret=0	# use default behavior
			;;
	esac

	return ${_ret}
}

# FreeBSD 11
# EXIT STATUS
#     Exit status indicates how the VM was terminated:
#     0       rebooted
#     1       powered off
#     2       halted
#     3       triple fault
# FreeBSD 12+
#     4       exited due to an error
# route for exit codes and  exit_action:
#   on_poweroff='destroy'
#   on_reboot='restart'
#   on_crash='destroy'
# return 0 when we should stop bhyve loop
# return 1 when show must go on
exit_action_mode()
{
	local _ret

	if [ ${exit_action} = "0" -a ${bhyve_exit} -eq 0 ]; then
		# no exit_action mode but normal reboot
		return 1
	fi

	[ "${exit_action}" != "1" ] && return 0

	# use on_poweroff/on_reboot/on_crash settings
	logger -t bhyverun.sh "in exit_action mode"
	_ret=0

	case ${bhyve_exit} in
		0)
			case "${on_reboot}" in
				restart)
					_ret=1
					;;
				destroy)
					_ret=0
					;;
				*)
					logger -t bhyverun.sh "unknown value for on_reboot: ${on_reboot}"
					_ret=1	# use default behavior
					;;
			esac
			logger -t bhyverun.sh "bhyve ${jname} was rebooted, exit_action_mode ret: ${_ret}"
			;;
		1)
			case "${on_poweroff}" in
				restart)
					_ret=1
					;;
				destroy)
					_ret=0
					;;
				*)
					logger -t bhyverun.sh "unknown value for on_poweroff: ${on_poweroff}"
					_ret=0	# use default behavior
					;;
			esac
			logger -t bhyverun.sh "bhyve ${jname} was poweroff, exit_action_mode ret: ${_ret}"
			;;
		2)
			on_crash_status
			_ret=$?
			logger -t bhyverun.sh "bhyve ${jname} was halted, exit_action_mode ret: ${_ret}"
			;;
		3)
			on_crash_status
			_ret=$?
			logger -t bhyverun.sh "bhyve ${jname} was tripple fault, exit_action_mode ret: ${_ret}"
			;;
		4)
			on_crash_status
			_ret=$?
			logger -t bhyverun.sh "bhyve ${jname} exited due to an error, exit_action_mode ret: ${_ret}"
			;;
		*)
			on_crash_status
			_ret=$?
			logger -t bhyverun.sh "bhyve ${jname} exited with unknown error ${bhyve_exit}, exit_action_mode ret: ${_ret}"
			;;
	esac
	return ${_ret}
}

while getopts "c:d:e:g:l:r:" opt; do
	case "${opt}" in
		c) conf="${OPTARG}" ;;
		d) debug="${OPTARG}" ;;
		e) exit_action="${OPTARG}" ;;
		g) debug_engine="${OPTARG}" ;;
		l) orig_logfile="${OPTARG}" ;;
		r) restore_checkpoint="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

[ -z "${debug_engine}" ] && debug_engine="none"
[ -z "${xhci}" ] && xhci=0
[ -z "${exit_action}" ] && exit_action=0	# use on_poweroff/on_reboot/on_crash settings: disabled by default
[ ! -f "${conf}" ] && exit 0
. ${conf}

# jailed process?
jailed=$( sysctl -qn security.jail.jailed 2>/dev/null )
[ -z "${jailed}" ] && jailed=0

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

[ -z "${cbsd_workdir}" ] && . /etc/rc.conf

if [ -z "${cbsd_workdir}" ]; then
	echo "No cbsd workdir defined"
	exit 1
else
	workdir="${cbsd_workdir}"
fi

. /usr/local/cbsd/cbsd.conf

freebsdhostversion=$( ${miscdir}/elf_tables --ver /bin/sh )
orig_vnc_args="${vnc_args}"

while [ ! -f /tmp/bhyvestop.${jname}.lock  ]; do

	vnc_args="${orig_vnc_args}"

	/usr/sbin/bhyvectl --vm=${jname} --destroy > /dev/null 2>&1
	/usr/bin/truncate -s0 ${vm_logfile}

	if [ ${cd_boot_once} -eq 0 ]; then
		echo "DEBUG: ${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
		eval "${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
	else
		echo "Boot from CD" | /usr/bin/tee -a ${vm_logfile}
		echo "DEBUG: ${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
		eval "${bhyveload_cmd}" | /usr/bin/tee -a ${vm_logfile}
	fi

	case "${vm_boot}" in
		"cd")
			# add ,wait args when boot from CD
			if [ "${vm_efi}" != "none" ]; then
				if [ -n "${vnc_args}" -a "${vm_vnc_port}" != "1" ]; then
					#orig_vnc_args="${vnc_args}"
					# bhyve_vnc_vgaconf before wait
					if [ "${bhyve_vnc_vgaconf}" != "io" ]; then
						[ -n "${bhyve_vnc_vgaconf}" ] && vnc_args="${vnc_args},vga=${bhyve_vnc_vgaconf}"
					fi
					#orig_vnc_args="${vnc_args}"
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

	if [ ${jailed} -eq 0 ]; then
		for i in ${mytap}; do
			/sbin/ifconfig ${i} up
		done
	fi

	[ ${freebsdhostversion} -lt 1100120 ] && vm_vnc_port=1 # Disable xhci on FreeBSD < 11

	if [ "${vm_efi}" != "none" ]; then
		if [ -n "${vm_vnc_port}" -a "${vm_vnc_port}" != "1" ]; then
			if [ ${xhci} -eq 1 ]; then
				xhci_args="-s 30,xhci,tablet"
			else
				xhci_args=
			fi
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

	if [ -n "${live_migration_args}" ]; then
		# check that this is for me
		if [ ! -r ${jailsysdir}/${jname}/live_migration.conf ]; then
			live_migration_args=
			break
		else
			. ${jailsysdir}/${jname}/live_migration.conf
			my_hostname=$( cat ${workdir}/nodename | awk '{printf $1}' )
			if [ "${my_hostname}" = "${live_migration_dst_nodename}" ]; then
				# this is for me!
				live_migration_args="-R ${live_migration_args}"
			else
				# this is not for me!
				live_migration_args=
			fi
		fi
	fi

	bhyve_cmd="env LIB9P_LOGGING=/tmp/cbsd_lib9p.log /usr/bin/nice -n ${nice} /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} ${add_bhyve_opts} ${hostbridge_args} ${virtio_9p_args} ${uefi_boot_args} ${dsk_args} ${dsk_controller_args} ${cd_args} ${nic_args} ${nvme_args} ${virtiornd_args} ${pci_passthru_args} ${vnc_args} ${xhci_args} ${lpc_args} ${console_args} ${efi_args} ${checkpoint_args} ${live_migration_args} ${jname}"
	debug_bhyve_cmd="env LIB9P_LOGGING=/tmp/cbsd_lib9p.log /usr/sbin/bhyve ${bhyve_flags} -c ${vm_cpus} -m ${vm_ram} ${add_bhyve_opts} ${hostbridge_args} ${virtio_9p_args} ${uefi_boot_args} ${dsk_args} ${dsk_controller_args} ${cd_args} ${nic_args} ${nvme_args} ${virtiornd_args} ${pci_passthru_args} ${vnc_args} ${xhci_args} ${lpc_args} ${console_args} ${efi_args} ${checkpoint_args} ${live_migration_args} ${jname}"

	echo "[debug] ${bhyve_cmd}"
	logger -t bhyverun.sh "[debug] ${bhyve_cmd}"
	echo "cmd: ${bhyve_cmd}" >> ${vm_logfile}
	echo "-----" >>  ${vm_logfile}

	bhyve_exit=0

	# triggering to update process id
	/usr/local/bin/cbsd task mode=new "sleep 5; /usr/local/bin/cbsd bset jname=${jname} vm_pid=auto"

	case "${debug_engine}" in
		gdb)
			if [ -x /usr/local/bin/gdb ]; then
				gdb_cmd="/usr/local/bin/gdb"
			elif [ -x /usr/libexec/gdb ]; then
				gdb_cmd="/usr/libexec/gdb"
			elif [ -x /usr/bin/gdb ]; then
				gdb_cmd="/usr/bin/gdb"
			fi
			# break while loop
			touch /tmp/bhyvestop.${jname}.lock
			echo
			echo "Warning"
			echo "Run bhyve throuch GDB. Please execute 'run' to launch bhyve instance"
			echo
			echo "/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock ${gdb_cmd} -batch -x /tmp/cmds.$$ --args ${debug_bhyve_cmd}"
			/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock ${gdb_cmd} -ex run --args ${debug_bhyve_cmd}
			bhyve_exit=$?
			/bin/rm -f /tmp/cmds.$$
			;;
		lldb)
			# break while loop
			touch /tmp/bhyvestop.${jname}.lock
			echo
			echo "Warning"
			echo "Run bhyve throuch LLDB. Please execute 'run' to launch bhyve instance"
			echo
			echo "/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock /usr/bin/lldb -- ${debug_bhyve_cmd}"
			/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock /usr/bin/lldb -- ${debug_bhyve_cmd}
			bhyve_exit=$?
			/bin/rm -f /tmp/cmds.$$
			;;
		*)
			/usr/bin/lockf -s -t0 /tmp/bhyveload.${jname}.lock ${bhyve_cmd} >> ${vm_logfile} 2>&1
			bhyve_exit=$?
			;;
	esac


	ret=0
	exit_action_mode
	ret=$?

	if [ ${ret} -eq 0 ]; then
		# exit from loop
		touch /tmp/bhyvestop.${jname}.lock
		echo "bhyve exit code: ${bhyve_exit}. exit_action settings: ${exit_action}, exit_action_mode ret: ${ret}: must stoppped"
		logger -t bhyverun.sh "bhyve exit code: ${bhyve_exit}. exit_action settings: ${exit_action}, exit_action_mode ret: ${ret}: must stopped"
		case ${bhyve_exit} in
			0|1)
				# normal exit
				/bin/rm -f ${vm_logfile}
				;;
			*)
				# bhyve error or crash
				echo "See ${vm_logfile} for details"
				echo
				/usr/bin/tail -n50 ${vm_logfile}
				echo "Sleep 1 seconds..."
				sleep 1
		esac
	else
		echo "bhyve exit code: ${bhyve_exit}. exit_action settings: ${exit_action}, exit_action_mode ret: ${ret}: must continue"
		logger -t bhyverun.sh "bhyve exit code: ${bhyve_exit}. exit_action settings: ${exit_action}, exit_action_mode ret: ${ret}: must continue"
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

# live migration todo
# check for bhyve migrated successful to me ( bregister and/or bstatus passed ? )

# bhyverun.sh QUEUE
[ -z "${cbsd_queue_name}" ] && cbsd_queue_name="/clonos/bhyvevms/"

if [ -x "${moduledir}/cbsd_queue.d/cbsd_queue" ]; then
	[ "${cbsd_queue_name}" != "none" ] && [ "${cbsd_queue_name}" != "none" ] && /usr/local/bin/cbsd cbsd_queue cbsd_queue_name=${cbsd_queue_name} id=${jname} cmd=bstop status=1 data_status=1
	sleep 0.3	# timeout for web socket
	[ "${cbsd_queue_name}" != "none" ] && /usr/local/bin/cbsd cbsd_queue cbsd_queue_name=${cbsd_queue_name} id=${jname} cmd=bstop status=2 data_status=0
fi

# extra destroy
/usr/bin/nice -n ${nice} /usr/sbin/bhyvectl --vm=${jname} --destroy > /dev/null 2>&1 || true
/bin/rm -f /tmp/bhyvestop.${jname}.lock
# extra stop/cleanup
/usr/local/bin/cbsd bstop cbsd_queue_name=none jname=${jname}

exit ${bhyve_exit}
