# CBSD XEN DOM0

## Introduction

Starting from **CBSD** 10.3.0 experimental support of [**XEN**](http://xenproject.org/) hypervisor has been added.

Since CBSD provides only a script wrapper to work with XEN, for their work you need to hold the system configuration in accordance with the documentation [**XEN FreeBSD Dom0**](http://wiki.xen.org/wiki/FreeBSD_Dom0)

To create and operate with XEN through the CBSD, use the prefix **x** for the corresponding commands: xconstruct-tui, xls, xstop, xstart, xremove

## Initialization

<details>
  <summary>Setup FreeBSD host</summary>

Based on: [https://wiki.freebsd.org/Xen](https://wiki.freebsd.org/Xen) and/or [https://wiki.xen.org/wiki/FreeBSD_Dom0](https://wiki.xen.org/wiki/FreeBSD_Dom0)

Hint: XEN/FreeBSD Dom0 boot not working in UEFI mode, please use legacy (MBR) boot method. Let us know if this is wrong information.

1) Install XEN software:

```
pkg install -y sysutils/xen-tools emulators/xen-kernel
sysrc xencommons_enable=YES
echo 'vm.max_user_wired=-1' >> /etc/sysctl.conf
echo 'xc0     /usr/libexec/getty Pc           xterm   onifconsole secure' >> /etc/ttys
mkdir -p /var/lock /var/run/xen
echo 'xen_cmdline="dom0_mem=2048M dom0_max_vcpus=2 dom0=pvh com1=115200,8n1 guest_loglvl=all loglvl=all vga=keep noreboot"' >> /boot/loader.conf
echo 'xen_kernel="/boot/xen"' >> /boot/loader.conf
```

</details>

<details>
  <summary>Setup Linux Debian host</summary>

Based on: [https://wiki.debian.org/Xen](https://wiki.debian.org/Xen):

1) Install XEN software:
```
install xen-system-amd64 xen-utils-common
```

2) (optional): edit /etc/default/grub.d/xen.cfg: GRUB_CMDLINE_XEN_DEFAULT="dom0_mem=2048M,max:2048M dom0_max_vcpus=2"


3) Update grub:
```
update-grub
```

</details>


## Create first guest:

* via TUI:

```
cbsd xconstruct-tui
```

* or via CLI:

```
cbsd xcreate ...
```

* or via CBSDfile:

```

```
