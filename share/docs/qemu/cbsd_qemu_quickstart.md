# **CBSD** QEMU quick start

[QEMU](https://www.qemu.org/) (Quick Emulator) is a free and open-source emulator that uses dynamic binary translation to emulate the processor of a computer. 

It provides a variety of hardware and device models for the machine, enabling it to run different guest operating systems. 

QEMU can be used in conjunction with Kernel-based Virtual Machine (KVM) and/or NVMM to execute virtual machines at near-native speeds. 

Additionally, QEMU supports the emulation of user-level processes, allowing applications compiled for one processor architecture to run on another.

QEMU supports the emulation of various processor architectures, including x86, ARM, PowerPC, RISC-V, and others.

QEMU/KVM/NVMM support matrix:

| Platform     | QEMU supported | NVMM supported | KVM supported |
| ------------ | -------------- | -------------- | ------------- |
| DragonflyBSD | Y              | Y              |       N       |
| FreeBSD      | Y              | N              |       N       |
| NetBSD       | Y              | Y              |       N       |
| Linux        | Y              | N              |       Y       |

<details>
  <summary>Setup Linux host</summary>

## CBSD + QEMU + Linux

</details>

Follow the [Quick start instructions for Linux](/share/docs/general/cbsd_quickstart.md#installation-on-linux).

<details>
  <summary>Setup FreeBSD host</summary>

## CBSD + QEMU + FreeBSD

Currently (at 2025) FreeBSD does not have QEMU acceleration, however, you can use QEMU  to emulate non-native architectures.

How-to-start:

1) Install the QEMU package ( [qemu](https://www.freshports.org/emulators/qemu/) or [qemu-devel](https://www.freshports.org/emulators/qemu-devel/) package ) then reinitialize CBSD ( prefer 'qemu-devel' package instead of 'qemu' ):
```
pkg install -y qemu-devel || pkg instal -y qemu
pkg install -y tmux
cbsd initenv
```

To test:
```
cbsd summary
```

Install additional software:

  - for ARM/aarch64 VM: `pkg install -y `
  - for RISCV-based VM: `pkg install -y opensbi u-boot-qemu-riscv64`
  - for x86-64 VM: `pkg install -y edk2-qemu-x64`

Create a vm using any of the three preferred methods:

2a) via TUI (dialog-based) interface:
```
cbsd qconstruct-tui
```

Choose target 'arch', 'vm_os_type', 'vm_os_profile' , 'jname' then 'GO'.

2b) via command line:
```
cbsd qcreate jname=vm1 vm_os_type=freebsd vm_os_profile=FreeBSD-riscv64-14.3 vm_ram=2g vm_cpus=1 runasap=1 imgsize=10g          # to create RISCV VM
cbsd qcreate jname=vm2 vm_os_type=freebsd vm_os_profile=FreeBSD-aarch64-14.3 vm_ram=2g vm_cpus=1 runasap=1 imgsize=10g          # to create aarch64 VM
```

2c) via CBSDfile:
```

```
```
cbsd up
```

</details>

<details>
  <summary>Setup DragonflyBSD host</summary>

## CBSD + QEMU + DragonflyBSD

**CBSD** version 13.0.18 added support for [QEMU](http://wiki.qemu.org/Main_Page) and [NVMM](https://blog.netbsd.org/tnf/entry/from_zero_to_nvmm) accelerator. Similar to the commands for jail, bhyve and XEN, you can create and manage QEMU-based virtual machines through similar commands and dialog forms, while the commands are prefixed with 'q': qstart, qdestroy ..

Currently NVMM acceleration is only available on the [DragonFlyBSD](https://www.dragonflybsd.org)/[NetBSD](https://www.netbsd.org) platform. However, if you are using CBSD and QEMU on FreeBSD, you can take advantage of the lightweight image of the alternate architectures as a full virtual machine if the [QEMU-usermode jail](/en/cbsd_qemu_usermode.html) is not for you. So, on a FreeBSD amd64 host, you can easily get up and running with an OS for ARMv8/aarch64 or RISCv64.

When using NVMM, make sure the module is loaded:

```
	kldload nvmm
```

Don't forget to add nvmm into auto-load via /boot/loader.conf:

```
	nvmm_load="YES"
```

You can check the status of NVMM through the command:

```
	nvmmctl identify
```

If the module is missing or does not work correctly, you can create VMs, but they will work without acceleration.

Also, acceleration will not be available if you are running non-native architectures.

You need to install QEMU package named 'qemu' (or 'qemu-devel' in some cases):

```
	pkg install qemu
```

Detailed description of working with NVMM/QEMU on the: [DragonflyBSD project page](https://www.dragonflybsd.org/docs/docs/howtos/nvmm/), [NetBSD project guide](https://www.netbsd.org/docs/guide/en/chap-virt.html)

</details>

When working with emulation of non-native architectures, you may need firmware and bios to boot systems:

For example:

* **u-boot-qemu-arm64** - Cross-build das u-boot for model qemu-arm64
* **u-boot-qemu-riscv64** - Cross-build das u-boot for model qemu-riscv64
* **opensbi** - RISC-V SBI bootloader and firmware

## [TODO/Limitation]()

At the moment, qemu is started from the 'root' user;

On the DragonFlyBSD platform, support for HammerFS2 in CBSD is under development: automatic work with PFS / snapshot / COW using HammerFS is currently not possible;

On FreeBSD platform, QEMU does not support the QXL/Spice protocol;

## [Creating a QEMU VM]()

You can work with QEMU-based virtual machines through the usual CBSD methods: TUI interface, CLI interface, or [CBSDfile](/en/cbsdfile.html).

```
	cbsd qconstruct-tui --help
	cbsd qcreate --help
	cbsd qstart --help
	cbsd qdestroy --help
```

## [Demo of working with QEMU via CBSD]()

Demo of NVMM/QEMU via CBSD on DragonflyBSD 6.1-dev, Demo of running ARMv8 / AARCH64 and RISCv64 virtual machines on FreeBSD amd64 / x86\_64:

[YouTube video player](https://www.youtube.com/embed/ACZ2dS1SRcc)
