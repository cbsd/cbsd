#!/bin/sh
bootable="-o bootimage=i386;$4/boot/cdboot -o no-emul-boot"
#bootable=""

if [ $# -lt 3 ]; then
echo "Usage: $0 image-label image-path-name path"
exit 1
fi

LABEL=$1
NAME=$2
PATH=$3
PUBLISHER="The CBSD Project. http://www.bsdstore.ru"

/usr/sbin/makefs -t cd9660 $bootable -o rockridge -o label=$LABEL -o publisher="${PUBLISHER}" $NAME $PATH
