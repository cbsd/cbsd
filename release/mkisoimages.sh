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

# tools/boot/install-boot.sh
# Minimum size of FAT filesystems, in KB.
fat32min=33292
fat16min=2100

# tools/boot/install-boot.sh
get_uefi_bootname()
{
	case ${TARGET:-$(uname -m)} in
		amd64) echo bootx64 ;;
		arm64) echo bootaa64 ;;
		i386) echo bootia32 ;;
		arm) echo bootarm ;;
		riscv) echo bootriscv64 ;;
		*) die "machine type $(uname -m) doesn't support UEFI" ;;
	esac
}

# tools/boot/install-boot.sh
make_esp_file()
{
	local file sizekb loader device stagedir fatbits efibootname

	file=$1
	sizekb=$2
	loader=$3

	if [ "$sizekb" -ge "$fat32min" ]; then
		fatbits=32
	elif [ "$sizekb" -ge "$fat16min" ]; then
		fatbits=16
	else
		fatbits=12
	fi

	stagedir=$(mktemp -d /tmp/stand-test.XXXXXX)
	mkdir -p "${stagedir}/EFI/BOOT"
	efibootname=$(get_uefi_bootname)
	cp "${loader}" "${stagedir}/EFI/BOOT/${efibootname}.efi"
	makefs -t msdos \
		-o fat_type=${fatbits} \
		-o sectors_per_cluster=1 \
		-o volume_label=EFISYS \
		-s ${sizekb}k \
		"${file}" "${stagedir}"
		rm -rf "${stagedir}"
}

# release/amd64/mkisoimages.sh
make_efi()
{
	local _md _tmpmnt

	echo " * efi"

#	bootable="-o bootimage=i386;${DPATH}/boot/cdboot -o no-emul-boot"
	# Make EFI system partition.
	espfilename=$(mktemp /tmp/efiboot.XXXXXX)
	# ESP file size in KB.
	espsize="2048"
	echo "+"
	make_esp_file ${espfilename} ${espsize} ${BASEBITSDIR}/boot/loader.efi
	echo "+"
#	bootable="$bootable -o bootimage=i386;${espfilename} -o no-emul-boot -o platformid=efi"

	echo " * efi done"

# legacy/old
#	truncate -s64M /tmp/efiboot.$$.img
#	#/bin/dd if=/dev/zero of=/tmp/efiboot.$$.img bs=4k count=200
#	_md=$( /sbin/mdconfig -a -t vnode -f /tmp/efiboot.$$.img )
##	/sbin/newfs_msdos -F 12 -m 0xf8 /dev/${_md}
#	/sbin/newfs_msdos -F 32 -c 1 -L EFISYS /dev/${_md}
#	_tmpmnt=$( mktemp -d )
#	/sbin/mount -t msdosfs /dev/${_md} ${_tmpmnt}
#	/bin/mkdir -p ${_tmpmnt}/efi/boot
#	/bin/cp ${CHROOT}/boot/loader.efi ${_tmpmnt}/efi/boot/bootx64.efi
#	/sbin/umount ${_tmpmnt}
#	/sbin/mdconfig -d -u ${_md}
#	/bin/rmdir ${_tmpmnt}
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
	echo "EFI enabled"
	#bootable="-o bootimage=i386;/tmp/efiboot.$$.img -o no-emul-boot -o bootimage=i386;${CHROOT}/boot/cdboot -o no-emul-boot"
	bootable="-o bootimage=i386;${DPATH}/boot/cdboot -o no-emul-boot"
	bootable="$bootable -o bootimage=i386;${espfilename} -o no-emul-boot -o platformid=efi"

else
	echo "EFI disabled"
	bootable="-o bootimage=i386;${CHROOT}/boot/cdboot -o no-emul-boot"
fi

#/usr/sbin/makefs -t cd9660 ${bootable} -o rockridge -o label="$LABEL" -o publisher="$publisher" "$NAME" "$@"
echo "/usr/sbin/makefs -t cd9660 ${bootable} -o rockridge -o label=${LABEL} -o publisher=\"${PUBLISHER}\" ${NAME} ${DPATH}"
/usr/sbin/makefs -t cd9660 ${bootable} -o rockridge -o label=${LABEL} -o publisher="${PUBLISHER}" ${NAME} ${DPATH}


if [ ${EFI} -eq 1 ]; then
	/bin/rm -f /tmp/efiboot.$$.img
	/bin/rm -rf ${espfilename}
fi

# release/amd64/mkisoimages.sh
if [ "$bootable" != "" ]; then
	# Look for the EFI System Partition image we dropped in the ISO image.
	for entry in `/usr/bin/etdump --format shell $NAME`; do
		eval $entry
		if [ "$et_platform" = "efi" ]; then
			espstart=`expr $et_lba \* 2048`
			espsize=`expr $et_sectors \* 512`
			espparam="-p efi::$espsize:$espstart"
			break
		fi
	done

	# Create a GPT image containing the partitions we need for hybrid boot.
	imgsize=`stat -f %z "$NAME"`
	/usr/bin/mkimg -s gpt \
		--capacity $imgsize \
		-b "${DPATH}/boot/pmbr" \
		-p freebsd-boot:="${DPATH}/boot/isoboot" \
		$espparam \
		-o hybrid.img

		# Drop the PMBR, GPT, and boot code into the System Area of the ISO.
		dd if=hybrid.img of="$NAME" bs=32k count=1 conv=notrunc
		rm -f hybrid.img
fi
