#!/bin/sh

# -l "LABEL"
# -n "NAME"
# -d "DPATH"
# -c "CHROOT"
# -p "PUBLISHER"
# -e "1|0" - enable EFI
while getopts "l:n:d:c:p:e:" opt; do
	case "$opt" in
		l) LABEL="${OPTARG}" ;;
		n) NAME="${OPTARG}" ;;
		d) DPATH="${OPTARG}" ;;
		c) CHROOT="${OPTARG}" ;;
		p) PUBLISHER="${OPTARG}" ;;
		e) EFI="${OPTARG}" ;;
	esac
	shift $(($OPTIND - 1))
done

make_efi()
{
	local _md _tmpmnt

	/bin/dd if=/dev/zero of=/tmp/efiboot.$$.img bs=4k count=100
	_md=$( /sbin/mdconfig -a -t vnode -f /tmp/efiboot.$$.img )
	/sbin/newfs_msdos -F 12 -m 0xf8 /dev/${_md}
	_tmpmnt=$( mktemp -d )
	/sbin/mount -t msdosfs /dev/${_md} ${_tmpmnt}
	/bin/mkdir -p ${_tmpmnt}/efi/boot
	/bin/cp ${CHROOT}/boot/loader.efi ${_tmpmnt}/efi/boot/bootx64.efi
	/sbin/umount ${_tmpmnt}
	/sbin/mdconfig -d -u ${_md}
	rmdir ${_tmpmnt}
}

[ -z "${LABEL}" ] && LABEL="NOLABEL"

if [ -z "${NAME}" ]; then
	echo "Empty NAME, use -n "
	exit 1
fi

if [ -z "${DPATH}" ]; then
	echo "Empty DPATH, use -d "
	exit 1
fi

[ -z "${PUBLISHER}" ] && PUBLISHER="The CBSD Project. http://www.bsdstore.ru"
[ -z "${EFI}" ] && EFI=1

if [ ! -f "${CHROOT}/boot/loader.efi" -a ${EFI} -eq 1 ]; then
	echo "Notes: You have no ${CHROOT}/boot/loader.efi. Disable EFI boot"
	efi=0
	EFI=0
fi

if [ ${EFI} -eq 1 ]; then
	make_efi
	bootable="-o bootimage=i386;/tmp/efiboot.$$.img -o no-emul-boot -o bootimage=i386;${CHROOT}/boot/cdboot -o no-emul-boot"
else
	bootable="-o bootimage=i386;${CHROOT}/boot/cdboot -o no-emul-boot"
fi

/usr/sbin/makefs -t cd9660 ${bootable} -o rockridge -o label=${LABEL} -o publisher="${PUBLISHER}" ${NAME} ${DPATH}
[ ${EFI} -eq 1 ] && /bin/rm -f /tmp/efiboot.$$.img
