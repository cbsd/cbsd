#!/bin/sh
# sqllist "this|is|  it" a b c
# echo $a
# sqllistdelimer for "|" alternative delimer
sqllist()
{
	local _i _str IFS _key _T

	_str="$1"
	if [ -n "${sqllistdelimer}" ]; then
		IFS="${sqllistdelimer}"
	else
		IFS="|"
	fi
	_i=2

	for _key in ${_str}; do
		eval _T=\${${_i}}
		_i=$((_i + 1))
		export ${_T}="${_key}"
	done
}

LOG="/tmp/notify.log"

date >> ${LOG}
echo $* >> ${LOG}

for i in $*; do
	export "${i}"
done

# skip update cmoomand
[ "${cmd}" = "update" ] && exit 0
cmd_status="${status}"
[ -z "${cmd_status}" ] && cmd_status="0"

SQL_GLOBAL="${workdir}/var/db/local.sqlite"
SQL_LOCAL="${workdir}/jails-system/${id}/local.sqlite"

jid=$( /usr/local/bin/sqlite3 ${SQL_GLOBAL} "SELECT jid FROM jails WHERE jname=\"${id}\"" )

if [ -r ${SQL_LOCAL} ]; then
	A=$( /usr/local/bin/sqlite3 ${SQL_LOCAL} "SELECT astart,vm_cpus,vm_ram,vm_os_type,vm_boot,vm_os_profile,vm_vnc_port,virtio_type,bhyve_vnc_tcp_bind,bhyve_vnc_resolution,cd_vnc_wait,protected,hidden,maintenance,ip4_addr,vnc_password,vm_hostbridge,vm_iso_path,vm_console,vm_efi,bhyve_generate_acpi,bhyve_wire_memory,bhyve_rts_keeps_utc,bhyve_force_msi_irq,bhyve_x2apic_mode,bhyve_mptable_gen,bhyve_ignore_msr_acc,bhyve_vnc_vgaconf,vm_cpu_topology,debug_engine FROM settings" )
	sqldelimer="|"
	sqllist "${A}" astart vm_cpus vm_ram vm_os_type vm_boot vm_os_profile vm_vnc_port virtio_type bhyve_vnc_tcp_bind bhyve_vnc_resolution cd_vnc_wait protected hidden maintenance ip4_addr vnc_password vm_hostbridge vm_iso_path vm_console vm_efi bhyve_generate_acpi bhyve_wire_memory bhyve_rts_keeps_utc bhyve_force_msi_irq bhyve_x2apic_mode bhyve_mptable_gen bhyve_ignore_msr_acc bhyve_vnc_vgaconf vm_cpu_topology debug_engine

	case "${jid}" in
		0)
			status=0
			;;
		*)
			status=1
			;;
	esac
else
	status=0
	astart=0
	vm_cpus=0
	vm_ram=0
	vm_vnc_port=0
	bhyve_vnc_tcp_bind="127.0.0.1"
fi

# start data field, open {
data="{\"node\":\"${node}\""

cat >>/tmp/${id}.json <<EOF
{
"jname": "${id}",
"command": "${cmd}",
"cmd_status": ${cmd_status},
"data": {
  "pid": ${jid},
  "status": ${status},
  "astart": ${astart},
  "vm_cpus": ${vm_cpus},
  "vm_ram": ${vm_ram},
  "vm_vnc_port": ${vm_vnc_port},
  "bhyve_vnc_tcp_bind": "${bhyve_vnc_tcp_bind}"
  }
}
EOF
