# CBSD Project

CBSD is a cross-platform wrapper for fast deployment of virtual machines and containers.

_Notes: Despite the fact that the project is focused on BSD-family OSes ( this is evident from the project name + check the [#Goals](#initial-mission) ), 
the port to GNU/Linux happened almost automatically as part of the work on supporting QEMU and netlink-based utilities such as ip(8);_

Tested/supported with following OSes and hypervisor/container engines:


| Platform[*1]                                     | [bhyve](/share/docs/bhyve/handbook.md) | [jail](/share/docs/jail/cbsd_jail_quickstart.md) | [QEMU](/share/docs/qemu/cbsd_qemu_quickstart.md) | [KVM](/share/docs/qemu/cbsd_qemu_quickstart.md) | [NVMM](/share/docs/qemu/cbsd_qemu_quickstart.md) | [XEN](http://www.xenproject.org/) |
|------------------------------------------------- | ----- | ---- | ------- | --- | ---- | --- |
| [DragonFlyBSD](https://www.dragonflybsd.org/)    |   -   |  y   |  y      |  -  |  y   |  -  |
| [FreeBSD](https://www.freebsd.org/)              |   y   |  y   |  y [*2] |  -  |  -   |  y  |
| [HardenedBSD](https://hardenedbsd.org/)          |   y   |  y   |  y      |  -  |  -   |  -  |
| [XigmaNAS](https://xigmanas.com/xnaswp/)         |   y   |  y   |  -      |  -  |  -   |  -  |
| GNU/Linux [*3]                                   |   -   |  -   |  y      |  y  |  -   |  -  |


:information_source: [^1] | The project welcomes the addition of new platforms. On our horizon: [OpenBSD](https://www.openbsd.org/), [NetBSD](http://netbsd.org/), MacOS, CGROUP/systemd-nspawn (as container engine);
:---: | :---
:information_source: [^2] | Can be used without acceleration: user-mode for jails and/or emulation of non-native architectures;
:information_source: [^3] | Linux support is experimental, currently the following distributions have been successfully tested: Debian, Ubuntu, Manjaro;
:---: | :---


## Usage

Quick start: [/share/docs/cbsd_quickstart.md](/share/docs/general/cbsd_quickstart.md)

Full guide: [/share/docs/docs.md](/share/docs/README.md)

# CBSD Project History and Goals

## Origins
The CBSD project was born in 2013 in response to a dismissive comment online claiming "Clouds can't be built on FreeBSD, Linux forever!" At the time, the Linux ecosystem was flourishing with numerous cloud and virtualization solutions from both major corporations and smaller companies, including *OpenVZ, Docker, Rancher, Kubernetes, LXD, OpenNebula, OpenStack, Proxmox, oVirt, XEN/XCP-NG, OpenShift, ISPPanel*, and many others.

## Initial Mission
From 2013 to 2025, our primary mission was to showcase FreeBSD's capabilities as a robust hosting platform. We embraced this challenge with enthusiasm, leading to the development of several key products designed to make FreeBSD more accessible to newcomers:

- [ClonOS](https://clonos.convectix.com)
- [MyBee](https://myb.convectix.com)
- [MyBee-QT](https://github.com/myb-project/mybee-qt)
- [NUBECTL](https://github.com/cbsd/nubectl)

This expansion led to significant script refinements, making the eventual addition of Linux support almost seamless. The project had grown beyond its FreeBSD roots, having earned significant recognition from the FreeBSD community—an achievement we continue to take pride in.

## Adapting to Modern Challenges
The IT landscape is rapidly evolving, presenting new challenges that demand innovative solutions. One prominent trend is the shift toward multi-cloud architectures, designed to mitigate risks associated with dependence on specific operating systems or hypervisors.

CBSD's next challenge is to provide platform-agnostic solutions, ensuring consistent operations across multiple operating systems and virtualization engines. This approach offers several advantages:

- **Vendor Independence**: If your preferred Linux distribution becomes less open or changes ownership (like Microsoft's acquisition), you can seamlessly switch to any BSD system.
- **Hardware Flexibility**: If BSD shows limitations on specific hardware, you can easily transition to any free Linux distribution.
- **Transparency**: Since CBSD functions as an argument generator for launching and controlling virtual environments, you maintain full visibility of the underlying commands and complete control over your data.

## The FreeBSD Ecosystem Today
The landscape has changed dramatically since CBSD's inception in 2013. While the project initially addressed a significant gap in FreeBSD solutions (with ezjail being the only notable alternative), the ecosystem has now become quite crowded. Today, numerous projects compete for the relatively small FreeBSD user base, offering similar capabilities.

A unique aspect of CBSD remains its integrated approach to managing both containers and virtual machines through a single interface—a feature that sets it apart from other solutions in the FreeBSD space.

[AppJail](https://github.com/DtxdF/AppJail), [bastillebsd](https://bastillebsd.org), [bhyve-rc](https://www.freshports.org/sysutils/bhyve-rc), [bhyvemgr](https://github.com/alonsobsd/bhyvemgr), [bsdploy](https://bsdploy.readthedocs.io/en/latest/), [bmd](https://github.com/yuichiro-naito/bmd), [bvm](https://github.com/bigdragonsoft/bvm), [chyves](http://chyves.org), [cirrina](https://gitlab.com/swills/cirrina), [cloudbsd](https://github.com/int0dh/CloudBSD), [crate](https://www.freshports.org/sysutils/crate), [ezjail](http://erdgeist.org/arts/software/ezjail/), [finch](https://github.com/dreamcat4/finch), [focker](https://github.com/sadaszewski/focker/), [fubarnetes](https://github.com/fubarnetes), [ioc](https://github.com/bsdci/ioc), iocage: ( [in shell](https://github.com/iocage/iocage_legacy), [in python](https://github.com/freebsd/iocage)), [iocell](https://github.com/bartekrutkowski/iocell), [iohyve](https://github.com/pr1ntf/iohyve), [jadm](https://github.com/NikolayDachev/jadm), [jail-primer](http://jail-primer.sourceforge.net/), [jailadmin](https://BSDforge.com/projects/sysutils/jailadmin/), [jailctl](http://anduin.net/jailctl/), [jailer (1)](https://www.freshports.org/sysutils/jailer/), [jailer (2)](https://github.com/illuria/jailer), [jailmanage](https://github.com/msimerson/jailmanage), [mailmanager](https://github.com/slicer69/jailmanager), [jailutils](http://thewalter.net/stef/freebsd/jails/jailutils/), [jest](https://github.com/tabrarg/jest), [jcreate](https://github.com/JohnKaul/jcreate), [jless](https://github.com/vermaden/jless), [jmore](https://github.com/vermaden/jmore), [kjail](https://github.com/Emrion/kjail), [kleened](https://github.com/kleene-project/kleened), [mkjail](https://github.com/mkjail/mkjail/), [pot](https://github.com/pizzamig/pot/), [pyvm-bhyve](https://github.com/yaroslav-gwit/PyVM-Bhyve), [HosterCore](https://github.com/yaroslav-gwit/HosterCore), [Sylve](https://github.com/AlchemillaHQ/Sylve), [quickjail](https://git.kevans.dev/kevans/quickjail), [qjail](http://erdgeist.org/posts/2017/dont-piss-in-my-beer.html), [quBSD](https://github.com/BawdyAnarchist/quBSD), [runj](https://github.com/samuelkarp/runj), [rvmadm](https://blog.project-fifo.net/rvmadm-managing-freebsd-jails/), [tredly](https://forums.freebsd.org/threads/introducing-tredly-containers-for-unix-freebsd.56016/), [vessel](https://github.com/ssteidl/vessel), [virt-manager](https://libvirt.org/drvbhyve.html), [vm-bhyve](https://github.com/freebsd/vm-bhyve), [vmstated](https://github.com/christian-moerz/vmstated), [warden](https://www.ixsystems.com/community/threads/warden-eol-and-iocage-jails-are-now-useless-what-do-we-do.70461/), [weasel](https://gitlab.com/swills/weasel), zjail, and other..

![FreeBSD-jail-chart-2025](https://convectix.com/img/freebsd-jail-chart-2025.png?raw=true)

![FreeBSD-bhyve-chart-2025](https://convectix.com/img/freebsd-bhyve-chart-2025.png?raw=true)


## Frontend

- QT6-based client for Linux/FreeBSD/MacOS/Window/Mobile: [MyBee-QT](https://github.com/myb-project/mybee-qt);
- NCURSES: [cbsd-tui](https://github.com/Peter2121/cbsd-tui);
- API-based: https://myb.convectix.com + [nubectl](https://github.com/cbsd/nubectl) (GOLANG-based client);

## Child project

The project is the core for such projects as:

- [ClonOS](https://clonos.convectix.com);
- [MyBee](https://myb.convectix.com);

CBSD Project also support online services:

- [CBSD Mirrors](https://github.com/cbsd/mirrors);

### Financial Contributors

Become a financial contributor and help us sustain our community. All funds are returned to the project (hosting, equipment, support) and invested in the development of a number of areas.

<a href="https://www.patreon.com/clonos"><img src="https://c5.patreon.com/external/logo/become_a_patron_button@2x.png" alt="Patreon donate button" /></a>

## Stargazers over time

[![Stargazers over time](https://starchart.cc/cbsd/cbsd.svg)](https://starchart.cc/cbsd/cbsd)
