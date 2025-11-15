# **CBSD** quick start

## Installation on FreeBSD

1) Install 'cbsd' package:
```
pkg install -y cbsd
```

## Installation on Linux

1) Install dependencies:

Debian/Ubuntu:
```
apt install -y sudo uuid-runtime bridge-utils net-tools gcc ovmf daemon psmisc make pkgconf pax rsync sharutils libsqlite3-dev libssh2-1-dev libssh2-1 libelf-dev libelf1 libbsd0 libbsd-dev qemu-system-x86 tmux dialog libsqlite3-dev sqlite3 curl libcurl4 libcurl4-openssl-dev libmagic-dev xorriso libedit-dev
```

Manjaro:
```
pacman -S sudo bridge-utils net-tools ovmf psmisc make pkg-config pax rsync sharutils libssh2 libelf libbsd qemu-system-x86 tmux dialog sqlite3 curl file xorriso cpio bind gnu-netcat git yay
```

2) Add 'cbsd' user and install 'cbsd' package:

:bangbang: | :warning: Please note! Linux is an experimental platform for CBSD. For this reason, CBSD is installed via tarball. The official package will be available after a period of testing.
:---: | :---

```
useradd cbsd
[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin
cd /usr/local
wget https://convectix.com/DL/cbsd.tgz
tar xfz cbsd.tgz
mv /usr/local/cbsd/bin/cbsd /usr/local/bin/
```

## Initialization

Initialize the default working directory (/usr/jails) with bridge-based virtual switch enabled (recommended for new users):
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

_Hint: for interactive initialization (advanced users), please read [CBSD init](initenv.md)_

If initialization was successful, the following command should return the version:
```
cbsd version
```

3) Depending on your capabilities (type on CLI: `cbsd summary`), follow the instruction:

- [first jail container](../jail/cbsd_jail_quickstart.md) (platform: FreeBSD, DragonFlyBSD, HardenedBSD, XigmaNAS);

- [first bhyve VM](../bhyve/cbsd_bhyve_quickstart.md) (platform: FreeBSD);

- [first XEN VM](../xen/cbsd_xen_quickstart.md) (platform: FreeBSD, Linux);

- [first QEMU VM (+NVMM, +KVM)](../qemu/cbsd_qemu_quickstart.md) (platform: FreeBSD, DragonFlyBSD, HardenedBSD, XigmaNAS, Linux);

