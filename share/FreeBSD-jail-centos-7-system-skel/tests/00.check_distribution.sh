#!/usr/local/bin/cbsd
# Wrapper for creating debootstrap environvent via 2 phases:
# 1) Get distribution into skel dir from FTP
# 2) Get distribution into data dir from skel dir

. ${subrdir}/nc.subr
. ${cbsdinit}
: ${distdir="/usr/local/cbsd"}
unset workdir

# MAIN
[ -z "${cbsd_workdir}" ] && . /etc/rc.conf
if [ -z "${cbsd_workdir}" ]; then
	echo "No workdir"
	exit 1
else
	workdir="${cbsd_workdir}"
fi
[ ! -r "${distdir}/cbsd.conf" ] && exit 1

. ${distdir}/cbsd.conf
. ${subrdir}/nc.subr
. ${system}
. ${strings}
. ${tools}
. ${subrdir}/fetch.subr

SRC_MIRROR="http://mirror.centos.org/centos/7.9.2009/os/x86_64/Packages"

rootfs_dir="${sharedir}/jail-centos-7-rootfs"

# based on chrooted list + glibc:
#mkdir -p /var/tmp/chroot/var/lib/rpm
#rpm --rebuilddb --root=/var/tmp/chroot
#wget http://mirror.centos.org/centos/7.9.2009/os/x86_64/Packages/centos-release-7-9.2009.0.el7.centos.x86_64.rpm
#rpm -i --root=/var/tmp/chroot --nodeps centos-release-7-9.2009.0.el7.centos.x86_64.rpm 
#yum --installroot=/var/tmp/chroot install -y rpm-build yum
LIST="
acl-2.2.51-15.el7.x86_64.rpm
audit-libs-2.8.5-4.el7.x86_64.rpm
basesystem-10.0-7.el7.centos.noarch.rpm
bash-4.2.46-34.el7.x86_64.rpm
binutils-2.27-44.base.el7.x86_64.rpm
bzip2-1.0.6-13.el7.x86_64.rpm
bzip2-libs-1.0.6-13.el7.x86_64.rpm
ca-certificates-2020.2.41-70.0.el7_8.noarch.rpm
centos-release-7-9.2009.0.el7.centos.x86_64.rpm
chkconfig-1.7.6-1.el7.x86_64.rpm
coreutils-8.22-24.el7.x86_64.rpm
cpio-2.11-28.el7.x86_64.rpm
cracklib-2.9.0-11.el7.x86_64.rpm
cracklib-dicts-2.9.0-11.el7.x86_64.rpm
cryptsetup-libs-2.0.3-6.el7.x86_64.rpm
cyrus-sasl-lib-2.1.26-23.el7.x86_64.rpm
dbus-1.10.24-15.el7.x86_64.rpm
dbus-libs-1.10.24-15.el7.x86_64.rpm
diffutils-3.3-5.el7.x86_64.rpm
dracut-033-572.el7.x86_64.rpm
dwz-0.11-3.el7.x86_64.rpm
elfutils-0.176-5.el7.x86_64.rpm
elfutils-default-yama-scope-0.176-5.el7.noarch.rpm
elfutils-libelf-0.176-5.el7.x86_64.rpm
elfutils-libs-0.176-5.el7.x86_64.rpm
expat-2.1.0-12.el7.x86_64.rpm
file-5.11-37.el7.x86_64.rpm
file-libs-5.11-37.el7.x86_64.rpm
filesystem-3.2-25.el7.x86_64.rpm
findutils-4.5.11-6.el7.x86_64.rpm
gawk-4.0.2-4.el7_3.1.x86_64.rpm
gdb-7.6.1-120.el7.x86_64.rpm
gdbm-1.10-8.el7.x86_64.rpm
glibc-2.17-317.el7.x86_64.rpm
glibc-common-2.17-317.el7.x86_64.rpm
gmp-6.0.0-15.el7.x86_64.rpm
gnupg2-2.0.22-5.el7_5.x86_64.rpm
gpgme-1.3.2-5.el7.x86_64.rpm
grep-2.20-3.el7.x86_64.rpm
groff-base-1.22.2-8.el7.x86_64.rpm
gzip-1.5-10.el7.x86_64.rpm
hardlink-1.0-19.el7.x86_64.rpm
info-5.1-5.el7.x86_64.rpm
json-c-0.11-4.el7_0.x86_64.rpm
keyutils-libs-1.5.8-3.el7.x86_64.rpm
kmod-20-28.el7.x86_64.rpm
kmod-libs-20-28.el7.x86_64.rpm
krb5-libs-1.15.1-50.el7.x86_64.rpm
libacl-2.2.51-15.el7.x86_64.rpm
libassuan-2.1.0-3.el7.x86_64.rpm
libattr-2.4.46-13.el7.x86_64.rpm
libcap-2.22-11.el7.x86_64.rpm
libcap-ng-0.7.5-4.el7.x86_64.rpm
libcom_err-1.42.9-19.el7.x86_64.rpm
libdb-5.3.21-25.el7.x86_64.rpm
libdb-utils-5.3.21-25.el7.x86_64.rpm
libffi-3.0.13-19.el7.x86_64.rpm
libgcc-4.8.5-44.el7.x86_64.rpm
libgcrypt-1.5.3-14.el7.x86_64.rpm
libgpg-error-1.12-3.el7.x86_64.rpm
libidn-1.28-4.el7.x86_64.rpm
libpwquality-1.2.3-5.el7.x86_64.rpm
libselinux-2.5-15.el7.x86_64.rpm
libsemanage-2.5-14.el7.x86_64.rpm
libsepol-2.5-10.el7.x86_64.rpm
libssh2-1.8.0-4.el7.x86_64.rpm
libstdc++-4.8.5-44.el7.x86_64.rpm
libtasn1-4.10-1.el7.x86_64.rpm
libuser-0.60-9.el7.x86_64.rpm
libutempter-1.1.6-4.el7.x86_64.rpm
libverto-0.2.5-4.el7.x86_64.rpm
libxml2-2.9.1-6.el7.5.x86_64.rpm
lua-5.1.4-15.el7.x86_64.rpm
lz4-1.8.3-1.el7.x86_64.rpm
ncurses-5.9-14.20130511.el7_4.x86_64.rpm
ncurses-base-5.9-14.20130511.el7_4.noarch.rpm
ncurses-libs-5.9-14.20130511.el7_4.x86_64.rpm
nss-pem-1.0.3-7.el7.x86_64.rpm
nss-3.44.0-7.el7_7.x86_64.rpm
nss-util-3.44.0-4.el7_7.x86_64.rpm
nspr-4.21.0-1.el7.x86_64.rpm
openldap-2.4.44-22.el7.x86_64.rpm
p11-kit-0.23.5-3.el7.x86_64.rpm
p11-kit-trust-0.23.5-3.el7.x86_64.rpm
pam-1.1.8-23.el7.x86_64.rpm
patch-2.7.1-12.el7_7.x86_64.rpm
pcre-8.32-17.el7.x86_64.rpm
perl-Carp-1.26-244.el7.noarch.rpm
perl-constant-1.27-2.el7.noarch.rpm
perl-Encode-2.51-7.el7.x86_64.rpm
perl-Exporter-5.68-3.el7.noarch.rpm
perl-File-Path-2.09-2.el7.noarch.rpm
perl-File-Temp-0.23.01-3.el7.noarch.rpm
perl-Filter-1.49-3.el7.x86_64.rpm
perl-Getopt-Long-2.40-3.el7.noarch.rpm
perl-HTTP-Tiny-0.033-3.el7.noarch.rpm
perl-parent-0.225-244.el7.noarch.rpm
perl-PathTools-3.40-5.el7.x86_64.rpm
perl-Pod-Perldoc-3.20-4.el7.noarch.rpm
perl-Pod-Simple-3.28-4.el7.noarch.rpm
perl-Pod-Usage-1.63-3.el7.noarch.rpm
perl-podlators-2.5.1-3.el7.noarch.rpm
perl-Scalar-List-Utils-1.27-248.el7.x86_64.rpm
perl-Socket-2.010-5.el7.x86_64.rpm
perl-srpm-macros-1-8.el7.noarch.rpm
perl-Storable-2.45-3.el7.x86_64.rpm
perl-Text-ParseWords-3.29-4.el7.noarch.rpm
perl-Thread-Queue-3.02-2.el7.noarch.rpm
perl-threads-1.87-4.el7.x86_64.rpm
perl-threads-shared-1.43-6.el7.x86_64.rpm
perl-Time-HiRes-1.9725-3.el7.x86_64.rpm
perl-Time-Local-1.2300-2.el7.noarch.rpm
pinentry-0.8.1-17.el7.x86_64.rpm
pkgconfig-0.27.1-4.el7.x86_64.rpm
popt-1.13-16.el7.x86_64.rpm
procps-ng-3.3.10-28.el7.x86_64.rpm
pth-2.0.7-23.el7.x86_64.rpm
pygpgme-0.3-9.el7.x86_64.rpm
pyliblzma-0.5.3-11.el7.x86_64.rpm
python-2.7.5-89.el7.x86_64.rpm
python-iniparse-0.4-9.el7.noarch.rpm
python-libs-2.7.5-89.el7.x86_64.rpm
python-pycurl-7.19.0-19.el7.x86_64.rpm
python-srpm-macros-3-34.el7.noarch.rpm
python-urlgrabber-3.10-10.el7.noarch.rpm
pyxattr-0.5.1-5.el7.x86_64.rpm
qrencode-libs-3.4.1-3.el7.x86_64.rpm
readline-6.2-11.el7.x86_64.rpm
redhat-rpm-config-9.1.0-88.el7.centos.noarch.rpm
rpm-4.11.3-45.el7.x86_64.rpm
rpm-build-4.11.3-45.el7.x86_64.rpm
rpm-build-libs-4.11.3-45.el7.x86_64.rpm
rpm-libs-4.11.3-45.el7.x86_64.rpm
rpm-python-4.11.3-45.el7.x86_64.rpm
sed-4.2.2-7.el7.x86_64.rpm
setup-2.8.71-11.el7.noarch.rpm
shadow-utils-4.6-5.el7.x86_64.rpm
shared-mime-info-1.8-5.el7.x86_64.rpm
sqlite-3.7.17-8.el7_7.1.x86_64.rpm
tar-1.26-35.el7.x86_64.rpm
unzip-6.0-21.el7.x86_64.rpm
ustr-1.0.4-16.el7.x86_64.rpm
xz-5.2.2-1.el7.x86_64.rpm
xz-libs-5.2.2-1.el7.x86_64.rpm
yum-3.4.3-168.el7.centos.noarch.rpm
yum-metadata-parser-1.1.4-10.el7.x86_64.rpm
yum-plugin-fastestmirror-1.1.31-54.el7_8.noarch.rpm
zip-3.0-11.el7.x86_64.rpm
zlib-1.2.7-18.el7.x86_64.rpm
nspr-4.21.0-1.el7.x86_64.rpm
rpm-4.11.3-45.el7.x86_64.rpm
grep-2.20-3.el7.x86_64.rpm
nss-softokn-3.44.0-8.el7_7.x86_64.rpm
python-nss-0.16.0-3.el7.x86_64.rpm
nss-devel-3.44.0-7.el7_7.x86_64.rpm
nss-softokn-freebl-3.44.0-8.el7_7.x86_64.rpm
nss-pam-ldapd-0.8.13-25.el7.x86_64.rpm
nss-pkcs11-devel-3.44.0-7.el7_7.x86_64.rpm
mod_nss-1.0.14-12.el7.x86_64.rpm
nss-util-3.44.0-4.el7_7.x86_64.rpm
nss-util-devel-3.44.0-4.el7_7.x86_64.rpm
nss-pem-1.0.3-7.el7.x86_64.rpm
apr-util-nss-1.5.2-6.el7.x86_64.rpm
openssl-libs-1.0.2k-19.el7.x86_64.rpm
libcurl-devel-7.29.0-59.el7.x86_64.rpm
curl-7.29.0-59.el7.x86_64.rpm
libcurl-7.29.0-59.el7.x86_64.rpm
glib2-2.56.1-7.el7.x86_64.rpm
"

[ -z "${jname}" ] && err 1 "${N1_COLOR}Empty jname${N0_COLOR}"
[ ! -d ${rootfs_dir} ] && ${MKDIR_CMD} -p ${rootfs_dir}

if [ ! -x /usr/local/bin/rpm2cpio ]; then
	err 1 "${N1_COLOR}No such rpm2cpio. Please ${N2_COLOR}pkg install rpm2cpio${N1_COLOR} it.${N0_COLOR}"
fi

for module in linprocfs fdescfs tmpfs linsysfs; do
	${KLDSTAT_CMD} -m "${module}" > /dev/null 2>&1 || ${KLDLOAD_CMD} ${module}
done

${KLDSTAT_CMD} -m linuxelf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux
${KLDSTAT_CMD} -m linux64elf > /dev/null 2>&1 || ${KLDLOAD_CMD} linux64

if [ ! -f ${rootfs_dir}/bin/bash ]; then
	export INTER=1

	if getyesno "Shall i download distribution via deboostrap from ${SRC_MIRROR}?"; then
		#${ECHO} "${N1_COLOR}debootstrap ${H5_COLOR}--include=openssh-server,locales,rsync,sharutils,psmisc,patch,less,apt --components main,contrib ${H3_COLOR}buster ${N1_COLOR}${rootfs_dir} ${SRC_MIRROR}${N0_COLOR}"
		cd ${rootfs_dir} || exit 1
		${MKDIR_CMD} pkgtmp
		for i in ${LIST}; do
			cd ${rootfs_dir}
			fetch -o pkgtmp/ ${SRC_MIRROR}/${i}
			/usr/local/bin/rpm2cpio pkgtmp/${i} | ${CPIO_CMD} -idmv 2>/dev/null
			${RM_CMD} -f pkgtmp/${i}
		done

		[ -d ${rootfs_dir}/etc/yum.repos.d ] && ${MV_CMD} ${rootfs_dir}/etc/yum.repos.d/ ${rootfs_dir}/etc/yum.repos.d-o
		${MKDIR_CMD} -p ${rootfs_dir}/etc/yum.repos.d
		${CAT_CMD} > ${rootfs_dir}/etc/yum.repos.d/CentOS-Base.repo <<EOF
[base]
name=CentOS-$releasever - Base
mirrorlist=http://mirrorlist.centos.org/?release=7&arch=x86_64&repo=os&infra=$infra
#baseurl=http://mirror.centos.org/centos/$releasever/os/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7
EOF

		${CAT_CMD} > ${rootfs_dir}/etc/resolv.conf <<EOF
nameserver 8.8.8.8
nameserver 8.8.4.4
EOF
	else
		echo "canceled"
		exit 1
	fi
fi

[ ! -f ${rootfs_dir}/bin/bash ] && err 1 "${N1_COLOR}no such distribution on ${N2_COLOR}${rootfs_dir}${N0_COLOR}"

. ${subrdir}/rcconf.subr
[ "${baserw}" = "1" ] && path=${data}

if [ ! -r ${data}/bin/bash ]; then
	${ECHO} "${N1_COLOR}populate jails data from: ${N2_COLOR}${rootfs_dir} ...${N0_COLOR}"
	# populate jails data from rootfs?
	. ${subrdir}/freebsd_world.subr
	customskel -s ${rootfs_dir}
fi

[ ! -f ${data}/bin/bash ] && err 1 "${N1_COLOR}No such ${data}/bin/bash"

exit 0
