#!/bin/sh
# see /usr/local/cbsd/share/bhyverun.sh --help
# MAIN()

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

echo "My config:"
grep -v '^#' ${conf} | sort
. ${conf}
echo
echo "------------"
# bhyve -c 2 -s 0,hostbridge -s 1,lpc -s 2,virtio-blk,/my/image -l com1,stdio -A -H -P -m 1G freebsd1 
set -o xtrace
${bhyve_cmd} -c ${vm_cpus} -m ${vm_ram} \
	${hostbridge_args} \
	${lpc_args} \
	${virtiornd_args} \
	${nic_args} \
	${dsk_args} \
	${cd_args} \
	${cd_args2} \
	${console_args} \
	${jname}

bhyve_exit=$?

exit ${bhyve_exit}
