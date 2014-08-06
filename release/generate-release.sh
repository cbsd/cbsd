#!/bin/sh

SRCDIR="/usr/src"
KERNEL="GENERIC"

export workdir="/usr/local/cbsd"

. ../cbsd.conf
. ../nc.subr

LABEL="CBSD_INSTALL"

if [ -z "${product}" ];
then
echo "No product name"
exit
fi

DSTDIR="/usr/local/${product}-fs"

[ ! -d ${DSTDIR} ] || chflags -R noschg ${DSTDIR} && rm -rf ${DSTDIR}

mkdir -p ${DSTDIR}
mtree -deU -f /etc/mtree/BSD.root.dist -p ${DSTDIR}
mtree -deU -f /etc/mtree/BSD.usr.dist -p ${DSTDIR}/usr
mtree -deU -f /etc/mtree/BSD.var.dist -p ${DSTDIR}/var
mtree -deU -f /etc/mtree/BSD.sendmail.dist -p ${DSTDIR}

set -e # Everything must succeed

# ORIG
make -C ${SRCDIR} buildworld installworld kernel KERNCONF="GENERIC" -DNO_CLEAN -j`sysctl -n hw.ncpu` DESTDIR="${DSTDIR}"
make -C ${SRCDIR} distribution DESTDIR="${DSTDIR}"

cat > ${DSTDIR}/boot/loader.conf << EOF
#geom_uzip_load="YES"
#rootfs_load="YES"
#rootfs_type="mfs_root"
#rootfs_name="/boot/mfsroot.gz"
#vfs.root.mountfrom="ufs:/dev/ufs/ROOTFS"
EOF

cat > ${DSTDIR}/etc/fstab << EOF
/dev/iso9660/${LABEL} / cd9660 ro 0 0
tmpfs /tmp tmpfs rw 0 0
EOF

cp ${DSTDIR}/root/.cshrc ${DSTDIR}/root/.cshrc-orig
cat > ${DSTDIR}/root/.cshrc << EOF
if( \`tty\` == "/dev/ttyv0" ) then
/usr/local/cbsd/release/install.sh
endif
EOF

echo "FreeBSD: CBSD Edition." > ${DSTDIR}/etc/motd

cat > ${DSTDIR}/etc/rc.conf << EOF
hostname="nodeXX.my.domain"
syslogd_flags="-ss -c"
rc_startmsgs="NO"
cleanvar_enable="NO"
tcp_extensions="NO"
tcp_keepalive="NO"
ppp_nat="NO"
syslogd_enable="NO"
sendmail_enable="NONE"
cron_enable="NO"
crashinfo_enable="NO"
check_quotas="NO"
clear_tmp_X="NO"
update_motd="NO"
virecover_enable="NO"
newsyslog_enable="NO"
mixer_enable="NO"
hostid_enable="NO"
EOF

cp ${DSTDIR}/etc/ttys ${DSTDIR}/etc/ttys-orig
grep -v ^ttyv0 /etc/ttys > ${DSTDIR}/etc/ttys
cat >> ${DSTDIR}/etc/ttys << EOF
ttyv0   "/usr/libexec/getty al.230400"         xterm   on  secure
EOF

mkdir -p ${DSTDIR}/usr/local ${DSTDIR}/usr/local/bin
cp -Rp /usr/local/cbsd ${DSTDIR}/usr/local

cp /usr/local/bin/rsync ${DSTDIR}/usr/local/bin/

/usr/local/cbsd/release/mkisoimages.sh -l ${LABEL} -n /usr/cbsd.iso -d ${DSTDIR}

mv ${DSTDIR}/root/.cshrc-orig ${DSTDIR}/root/.cshrc
mv ${DSTDIR}/etc/ttys-orig ${DSTDIR}/etc/ttys
