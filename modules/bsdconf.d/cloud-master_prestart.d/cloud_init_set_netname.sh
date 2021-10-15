#!/usr/local/bin/cbsd
# Helper to set proper interface name for cloud-init network settings
# this can be useful in distributions where interface names are 
# dynamically dependent on PCI bus slot
# Additional: https://www.bsdstore.ru/en/articles/cbsd_cloud_init.html

set -e
. ${distdir}/cbsd.conf
. ${subrdir}/nc.subr
. ${cbsdinit}
set +e

export NOCOLOR=1
NETWORK_CONFIG="${jailsysdir}/${jname}/cloud-init/network-config"
[ ! -r ${NETWORK_CONFIG} ] && exit 0

seq=0

for i in $( cbsd bpcibus jname=${jname} mode=get device_name=virtio-net ); do
	iface_index=$( echo ${i} | ${AWK_CMD} '{ printf $1 }' )
	[ -z "${iface_index}" ] && err 0 "cloud_init_set_netname.sh helper for $jname: unable to get iface index, skipp"
	#todo: ci_interface_name ( default_ci_interface_name )
	iface_name="enp0s${iface_index}"
	#echo "${seq} $iface_name"
#       ${SED_CMD} -i '' -e "s/name:.*\$/name: ${iface_name}/g"  -e "s/mac_address:.*\$/mac_address: ${nic_hwaddr0}/g"  ${NETWORK_CONFIG}
	${SED_CMD} -i '' -e "s/name: eth${seq}*\$/name: ${iface_name}/g"  -e "s/mac_address:.*\$/mac_address: ${nic_hwaddr0}/g"  ${NETWORK_CONFIG}
	seq=$(( seq + 1 ))
done

exit 0
