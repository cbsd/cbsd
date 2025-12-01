# **CBSD** quick start

## Installation

CBSD works on multiple platforms. Follow the relevant instructions:

<details>
  <summary>Installation on FreeBSD</summary>

1) Install 'cbsd' package:
```
pkg install -y cbsd
```

</details>

<details>
  <summary>Installation on Linux</summary>

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
</details>

<details>
  <summary>Installation on DragonflyBSD</summary>

1) Install 'cbsd' package:
```
pkg install -y cbsd
```

or via dports:
```
make -C /usr dports-create
env BATCH=no make -C /usr/ports/sysutils/cbsd install
```
</details>

## Initialization

Initialize the default working directory (/usr/jails) with bridge-based virtual switch enabled (recommended for new users):
```
/usr/local/cbsd/sudoexec/initenv /usr/local/cbsd/share/initenv.conf default_vs=1
```

_Hint: for interactive initialization (advanced users), please read [CBSD init](initenv.md)_

_Hint: `default_vs=1` Forces cbsd to create a bridge on your system at startup. If you want to manage the interfaces themselves, do not use this parameter._

If initialization was successful, the following command should return the version:
```
cbsd version
```

If you want to change the `initenv` parameter after initialization, use `cbsd initenv-tui` command.

Depending on your capabilities (type on CLI: `cbsd summary`), follow the instruction:

- [first jail container](../jail/cbsd_jail_quickstart.md) (platform: FreeBSD, DragonFlyBSD, HardenedBSD, XigmaNAS);

- [first bhyve VM](../bhyve/handbook.md) (platform: FreeBSD, XigmaNAS);

- [first XEN VM](../xen/cbsd_xen_quickstart.md) (platform: FreeBSD, Linux);

- [first QEMU VM (+NVMM, +KVM)](../qemu/cbsd_qemu_quickstart.md) (platform: FreeBSD, DragonFlyBSD, HardenedBSD, XigmaNAS, Linux);

## Help

Most of the documentation is embedded directly into the scripts. You can access it using the help argument:

```
cbsd <cmd> --help
```

To get a list of all commands or for a specific module:

```
cbsd help
cbsd help module=bhyve
```
