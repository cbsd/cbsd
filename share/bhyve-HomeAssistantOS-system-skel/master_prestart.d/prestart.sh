#!/bin/sh
zvol_dsk0_dev=$( readlink ${dsk0_path} )
zvol_dsk0=$( echo ${zvol_dsk0_dev} | sed s#/dev/zvol/## )
/sbin/zfs set volmode=full $zvol_dsk0
/sbin/gpart recover $zvol_dsk0_dev
mount_dir=$( mktemp -d )
trap "rmdir ${mount_dir}" HUP INT ABRT BUS TERM EXIT
mount -t msdosfs -o rw ${zvol_dsk0_dev}p1 ${mount_dir}
mkdir -p ${mount_dir}/CONFIG/network
cat <<- EOF > ${mount_dir}/CONFIG/network/cbsd-network
	[connection]
	id=CBSD network
	uuid=$(uuidgen)
	type=802-3-ethernet
	autoconnect=true

	[ipv4]
	address1=${ip4_addr}
	dns="8.8.8.8;8.8.4.4"
	gateway=${ci_gw4}
	method=manual
EOF
ls -l ${mount_dir}/CONFIG/network/
cat -n ${mount_dir}/CONFIG/network/cbsd-network
cp ~cbsd/.ssh/authorized_keys ${mount_dir}/CONFIG/
umount ${mount_dir}

exit 0
