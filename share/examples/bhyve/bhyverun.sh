#!/bin/sh
# alternative bhyve boot sample (via bhyveload)
# see /usr/local/cbsd/share/bhyverun.sh --help
# MAIN
while getopts "c:d:e:g:hl:r:w:" opt; do
	case "${opt}" in
		c) conf="${OPTARG}" ;;
		d) debug="${OPTARG}" ;;
		e) exit_action="${OPTARG}" ;;
		g) debug_engine="${OPTARG}" ;;
		l) orig_logfile="${OPTARG}" ;;
		r) restore_checkpoint="${OPTARG}" ;;
		w) workdir="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

if [ ! -r ${conf} ]; then
	echo "no conf"
	exit 1
fi

echo "-- my config--"
grep -v '^#' ${conf} | sort
. ${conf}
echo "--------------"

# we need to parse CBSD variables to convert into bhyve(8) key in the same way
# as original /usr/local/cbsd/share/bhyverun.sh

add_bhyve_opts="-H"  # Yield the virtual CPU thread when a HLT instruction is detected.
[ "${bhyve_generate_acpi}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -A"
[ "${bhyve_wire_memory}" = "1" -o -n "${pci_passthru_args}" ] && add_bhyve_opts="${add_bhyve_opts} -S"
[ "${bhyve_rts_keeps_utc}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -u"
[ "${bhyve_force_msi_irq}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -W"
[ "${bhyve_x2apic_mode}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -x"
[ "${bhyve_mptable_gen}" = "0" ] && add_bhyve_opts="${add_bhyve_opts} -Y" # disable mptable gen
[ "${bhyve_ignore_msr_acc}" = "1" ] && add_bhyve_opts="${add_bhyve_opts} -w"
[ -n "${uuid}" -a "${uuid}" != "0" ] && add_bhyve_opts="${add_bhyve_opts} -U ${uuid}"

set -o xtrace

# boot via bhyveload
case "${vm_boot}" in
	cd)
		/usr/sbin/bhyveload -c stdio -m ${vm_ram} -d ${cd_0_path} ${jname}
		;;
	hdd)
		/usr/sbin/bhyveload -c stdio -m ${vm_ram} -d ${dsk_0_path} ${jname}
		;;
esac

# via /usr/sbin/bhyve:
env cbsd_workdir="${workdir}" ${tmuxcmd} -2 -u new -d "${bhyve_cmd} \
	${add_bhyve_opts} \
	-c ${vm_cpus} \
	-m ${vm_ram} \
	${hostbridge_args} \
	${lpc_args} \
	${virtiornd_args} \
	${nic_args} \
	${dsk_args} \
	${cd_args} \
	${cd_args2} \
	${console_args} \
	${jname}"

bhyve_exit=$?
exit ${bhyve_exit}
