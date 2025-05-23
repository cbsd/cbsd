# What You Need to Know about CBSD

## Contents
- [Introduction](#introduction)
- [Layout](#layout)
- [Modules](#modules)
- [Configuration](#configuration)
- [Networking](#networking)
- [Support](#support)

## Introduction

**CBSD** is an additional layer of abstraction for the [jail(8)](http://www.freebsd.org/cgi/man.cgi?query=jail&sektion=8) framework, [bhyve hypervisor](http://www.freebsd.org/cgi/man.cgi?query=bhyve&sektion=8),[XEN project hypervisor](http://www.xenproject.org/) and some components of the [FreeBSD Operating System](https://www.freebsd.org/) used to make jails functional like other container management system used for application and service deployment and isolation.

The additional functionality **CBSD** provides relys on the following:

-   [vnet (VIMAGE)](https://klarasystems.com/articles/virtualize-your-network-on-freebsd-with-vnet/)
-   [zfs](https://docs.freebsd.org/en/books/handbook/zfs/)
-   [racct/rctl](https://klarasystems.com/articles/controlling-resource-limits-with-rctl-in-freebsd/)
-   [pf/ipfw/ipfilter](https://docs.freebsd.org/en/books/handbook/firewalls/)
-   [carp](https://docs.freebsd.org/en/books/handbook/advanced-networking/)
-   [hastd](https://wiki.freebsd.org/HighlyAvailableStorage)
-   [vale](https://man.freebsd.org/cgi/man.cgi?query=vale&sektion=4&manpath=FreeBSD+12.0-RELEASE+and+Ports)
-   [vxlan](https://wiki.freebsd.org/vxlan)

While many of these subsystems are not directly related to **jails** or **vm hypervisor**, **CBSD** uses these components to provide system administrators a more advanced, integrated system in which to implement solutions for issues faced in today's IT envirnonment. This page will provide information to help system administrators familiarize themselves with CBSD. While this page is not intended to be a comprehensive, all encompassing how-to, it will provide details about where files are stored, and how to use **CBSD** to manage and interact with the virtual environment.

Although **CBSD** aims to be the most user-friendly application (for example, using bsdconfig-style dialogs), the platform has evlolved into a comprehensive embedded virtual environment management system thatcan be used at the lowest level to create cloud infrastructure.

Engineers can work directly with **CBSD** as an end user interactively or, aternatively, can use **CBSD** as a middle abstraction layer, interacting with it through other applications at a higher level of abstraction.

**CBSD** assumes the use of multiple servers (cluster), but it can work equally well in a standalone version on a workstation or laptop.

The information provided here assumes a basic understanding of jails, how they are used, and how they are managed in FreeBSD. If you plan to work with containers, the official documentation about jails is a highly recommended starting point, and can be found in Chapter 14 of the FreeBSD Handbook:[Jails](http://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/jails.html). The [jail(8)](http://www.freebsd.org/cgi/man.cgi?query=jail&sektion=8) manpage is also a great resource.

If you are working with bhyve or XEN, be sure to first try to read the official documentation: [Chapter 21. Virtualization: FreeBSD as a Host with bhyve](https://www.freebsd.org/doc/handbook/virtualization-host-bhyve.html) and [XEN project hypervisor](http://www.xenproject.org/).

Before getting started, you should be familiar following terminology:

-   **Node (host): **A physical server that hosts the virtual environment.
-   **Jail (guest): **An isolated environment, complete with its own set of    software and services. A jail is able to run any software that is    available to the OS installed in the jail (cli or graphical).
-   **Cloud:** A farm/cluster of interconnected nodes, or a full-fledged  peer network 
-   **Base:** In the context of **CBSD**, a copy of the files in the  FreeBSD base system.
-   **CBSD:** A system for configuring and controlling node(s), jails, vms and certain subsystems of FreeBSD. CBSD provides a unified way to interact with and perform actions on the specified nodes or jails via the provided API. **CBSD** also provides the ability to implement and use [ACLs](https://www.freebsd.org/doc/handbook/fs-acl.html), and change permissions on specified resources.
-   **$workdir:** The working directory on a **CBSD** node that is initialized via the *cbsd initenv* command on the initial run. This directory is **/usr/jails** unless otherwise specified.
-   **$jname:** The name of a jail in the **CBSD** environment.

A quick word about jails. As stated, most software available to the OS the jail runs on can run inside of a jail. Server-side components such as DNS, Apache/nginx, postfix, etc. can run  inside of a jail, isolated from the host. Perhaps lesser known is graphical environments/applications can also run inside a jail isolated from the host. For example, a jail run an XServer or VNCServer, and then connected to using Xforwarding *firefox -display=REMOTEADDR:PORT*. There is also [xjails](https://www.bsdstore.ru/en/xorg_in_jail.html), Xorg running inside a jail isolated from the host.

## Layout

#### Summary of the **CBSD** filesystem hierarchy 

| Directory Path | Description    |
| --------- | -------- |
| \${workdir}/.rssh/   |This directory stores the private keys of remote nodes. The files are added and removed via the command **cbsd node** |
| \${workdir}/.ssh     | This directory stores the private and public keys of the nodes. The directory is created during initialization with the command ***cbsd initenv***. This is also where the public key comes from when the command **cbsd node mode=add** is issued to copy the pub key to a remote host. The Key file name is the md5 sum of the nodename.|
| \${workdir}/basejail | This directory is used to store the bases and kernels for FreeBSD that are used when creating baserw=0 jails. These are generated via cbsd buildworld/buildkernel, cbsd installworld/installkernel, or cbsd repo action=get sources=base/kernel)    |
| \${workdir}/etc      | Configuration files needed to run **CBSD**|
| \${workdir}/export   | The default directory that will be stored in a file exported by the jail (a cbsd jexport jname=$jname, this directory will file $jname.img)     |
| \${workdir}/import   | The default directory containing data to be imported to a jail (a cbsd jimport jname=$jname, will be deployed jail $jname)
| \${workdir}/jails    | This directory contains the mount point for the root jails that use baserw=0. |            
| \${workdir}/jails-data     | This directory stores all jail data. Backup these directories to take a backup of the jails (including fstab and rc.conf files). Note: if a jail uses baserw=1, these directories are the root of the jail when it starts. |
| \${workdir}/jails-fstab    |The fstab file for the jails. The syntax for regular FreeBSD with the only exception that the path to the mount point is written relative to the root *jail* (record **/usr/ports /usr/ports nullfs rw 0 0** in the file fstab.\$jname means that of the master node directory /usr/ports will be mounted at startup in \${workdir}/jails/$jname/usr/ports) |
| \${workdir}/jails-rcconf   |rc.conf files for jail creation. These parameters can be changed using \$editor, or via the command ***cbsd jset \$jname param=val*** (eg *cbsd jset jname=$jname ip=\"192.168.0.2/24\"*). To change these settings, the jail should **not** be running. |
|\${workdir}/jails-system    |This directory may contain some helper scripts related to the jail (eg wizards to configure, configurators, etc) as well as the preserved jail traffic when using ipfw and its description. This catalog participates in jimport/jexport operations and migration of jail |
|\${workdir}/var             |This directory contains system information for **CBSD**. For example, in ${workdir}/var/db is an inventory of local and remote nodes that were added. |
|/usr/local/cbsd             |A copy of the original files installed by the **CBSD** port. The working scripts for sudoexec can also be found here. |

#### Directory Structure
The largest directory used by **CBSD** is where all of the data **CBSD** uses is stored. This is the directory defined in the environment variable **$workdir**, and is a symlink to ***/usr/jails*** by default. This directory can be changed when necessary. $workdir is also the **CBSD** user's home directory. 

```
cd ~cbsd 

├──($workdir)->
	├── .rssh     # the private keys of remote nodes
	├── .ssh      # the private and public keys of the nodes
	├── basejail  # FreeBSD bases and kernels for baserw jails
	├── etc       # CBSD configuration files
	├── export    # image files create by jexport command 
	├── formfile
	├── ftmp
	├── import
	├── jails
	├── jails-data
	├── jails-fstab
	├── jails-rcconf
	├── jails-system
	├── modules
	├── nodes
	├── share
	├── src
	├── tmp
	├── var
	└── vm
```

There are two main directories used to store jail data. The deciding factor for which directory is used depends on whether or not a newly created jail should be able to write to it's base or not. This option is specified by passing the flag **baserw=0 or baserw=1** when creating a new jail.

To create a jail with a read-only base, pass the flag **baserw=0**. Instead of writing to the base, the new jail will use the standard base from **\$workdir/basejail/\$basename**. Jails with a read only base are stored in the directory **\$workdir/jails/$jname**. Any baserw=0 jail will mount the $basename through [nullfs](https://man.freebsd.org/cgi/man.cgi?mount_nullfs). This allows for the easy upgrade of all baserw=0 jails, as upgrading the $basename jail upgrades all of the jails using it. Another advantage is the fact that if a read only jail is compromised, the attacker will be unable to modify anything in base as it is read only.

When a new jail is created with the flag **baserw=1**, the jail will have the ability to write to it's own base. Jails with this ability store data in the directory ***\$workdir/jails-data/$jname***.

**CBSD** uses the standard directories specified by jail(8). This allows jails to migrated to or from any other jail management system that also follows the standards set by jail. The goal for the directories where jails are stored is to be consistent, and adhere to the jail standards. This allows for the greatest compatibility.

***Note:*** When using the jail type md, the directory \$workdir/jails-data/$jname will contain the image of the jail.

***Note:*** When using ZFS, CBSD has the ability to unmount a jail's data directory while the the jail is inactive. If a jail's data directory is found to be empty, don't panic. (At least when the jail is inactive). 

Check the output of the command:

``` sh
zfs list
```

To access the data use;

``` sh
zfs mount $jname_file_system
```

The second-largest directory in the **CBSD** hierarchy is *$workdir/var/db/*. This directory is where the configuration files for all of the jails created are stored. All jail settings are stored in the **jails** table in an SQLite3 database. The symbolic link **\${workdir}/var/db/local.sqlite** should always point to the correct/current database. The table schema is described in the file **\${workdir}/share/local-jails.schema**. SQLite3 can be used to query information about all jails on a node. 

For example, to see all jails on the node, and their IP address' execute;

``` sh
sqlite3 /usr/jails/var/db/local.sqlite "SELECT jname, ip4_addr FROM jails;"
```

Internal information for **CBSD** is stored in the $workdir/var/db directory. For example: The information on the list of added nodes,inventory of both the local and remote nodes, and so on.

The **$workdir/jails-system/** directory serves as additional storage for **CBSD** jail data. For example: There may be configuration services (puppet), files with the description of the jails, traffic statistics, resources statistics, and so on. 

One important thing to note in regards to security are the directories **\${workdir}/.rssh** and **${workdir}/.ssh**. These dirs contain the private RSA keys for the remote user **CBSD** nodes (.rssh) and the local nodes (.ssh). Make sure that the data in these directories are not available to other users of the system. For more information, please see the article about [GELI encryption](https://github.com/cbsd/cbsd/blob/develop/share/docs/general/cbsd_geli.md). By default, the key can be read only by a system **CBSD** user.

Finally, be sure to read about the modifications that **CBSD** makes to the system. This [page](https://github.com/cbsd/cbsd/blob/develop/share/docs/general/custom_freecbsd.md) describes all of the modifications that are carried out by **CBSD** scripts after installing on a FreeBSD system.


## Modules

The functionality of **CBSD** can be extended by activating additional modules that can be written by anyone.

Each module is a directory located in the **/usr/local/cbsd/modules** path. To activate a module, you must add its name into the **\~workdir/modules.conf** configuration file and re-initialize **CBSD** with:
 
```sh
 cbsd initenv
```
A good example of using an added module is the [ClonOS project](https://clonos.convectix.com/), which adds a web based gui, adds a VNC terminal to jails and vms, adds notifications via web socket transport and add helpers for deploying services in containers, etc (similar to the Linux based Proxmox).

## Configuration

**CBSD** is a highly configurable and customizable framework, which makes it an extremely flexible and versatile solution which can be used in a wide range of tasks.

Take a look at the contents of the **$workdir/etc/defaults/** directory. It contains default global settings (color scheme, logging setup) and/or configuration files for single commands (blogin, bstart, jclone, etc).

Settings can be reassigned in the FreeBSD-way, by writing changes to the file of the same name, but placing it one directory level in the **$workdir/etc/** directory. Similar to FreeBSD system configuration files in **/etc** and **/etc/defaults/**. [FreeBSD Configuration](https://docs.freebsd.org/en/books/handbook/config/#_the_etc_directory)

#### ANSII Color

**CBSD** displays output using colorized text by default using ANSII escape sequences. Doing so helps important information standout. If the colors are found to be unpleasant, or interfere with using output from commands or utilities available in **CBSD**, colors can be disabled by setting the environment variable NOCOLOR=1. 

For example, issuing the command;

``` sh
env NOCOLOR=1 cbsd jls
```

will disable the use of color in the output of the names of the jails.

### Opreations in multiple jails using jname as mask

Most of the **CBSD** commands support the jname paremeter. The value passed to jname allows wildcard expansion.

For example, if you want to perform the same operation on a group of jails (e.g: jail1, jail2, jail3), you can use **jname='jail\*'**

More examples:

```sh
cbsd jset jname='*' ver=native
cbsd jset jname='*' ver=native astart=0 devfs_ruleset=4
cbsd jexec jname='jail*' file -s /bin/sh
cbsd pkg jname='myja*l*' mode=install  ca_root_nss nss
cbsd jstop jname='*'
cbsd jstart jname='lala*'
```

![](http://www.convectix.com/gif/jnamemask.gif)

### Backups

#### Backing up the CBSD virtual environment.

Taking a backup, any sys admin worth their salt would agree,  is a must to ensure data is safe. To properly backup the virtual environments on the node, the following directories must be included (The description of each of these directories is in the
table above).

-   ${workdir}/var/db
-   ${workdir}/jails-fstab
-   ${workdir}/jails-system
-   ${workdir}/jails-data

## Networking

#### Expose command: tcp/udp port forwarding from  host to jail

The **CBSD** expose command can be used to forward tcp/udp ports from the host to  a guest (jail).

For example:

```sh
cbsd expose jname=test2 mode=add in=200 out=200
cbsd expose jname=test2 mode=delete in=200 out=200
cbsd expose jname=test2 mode=list
cbsd expose jname=test2 mode=clear
cbsd expose jname=test2 mode=flush
```    


**CBSD** uses the **fwd** ruleset of **ipfw** to configure port
forwarding. **CBSD** sets the number of counters in the **2001 - 2999**
range. This range can easily be changed in cbsd.conf if need be. Again,
always be mindful when changing firewall rules. Make sure no rules
conflict with the range configrured for **CBSD** to use.

Read more about [expose](https://github.com/cbsd/cbsd/blob/develop/share/docs/general/wf_expose_ssi.md).

#### Counting jail traffic

**CBSD** uses the **count** ruleset of [**ipfw**](https://www.freebsd.org/doc/en/books/handbook/firewalls-ipfw.html) filter to count jail traffic. **CBSD** sets the number of counters in the **99 --- 2000** range. The range can be easily adjusted in cbsd.conf if this interfes with existing rules. Be mindful when changing firewall rules. **CBSD** \"takes ownership\" of the rules in the range given. In otherwords, if there are other rules already in place using the specified range, there is the posibility that **CBSD** could delete and re-add the rules in the range. This means all rules in the range would be deleted, but only the **CBSD** rules would be added back in.

Read more about [counting jail traffic](https://github.com/cbsd/cbsd/blob/develop/share/docs/jail/wf_jailtraffic_ssi.md)

### About rsync-based copying jail data between nodes]

**CBSD** offers a wrapper to rsync called cbsdrsyncd. If **cbsdrsyncd** is activated, please keep in mind that there is the standard **rsyncd(1)** daemon running that looks at the specified *$jail-data* directory, and is protected by the rsync password. **CBSD** generates a strong password via the following command;

``` sh
head -c 30 /dev/random | uuencode -m - | tail -n 2 | head -n1 
```

**CBSD** transmits data through the rsync daemon over port 1873/tcp. Please secure this port from any traffic excpet for remote **CBSD**, or use encrypted communication between the nodes using something like IPSec.


## Support

### Encountering Problems

While the **CBSD** project strives to be bug free, like any software, bugs happen. If a component or tool that is part of **CBSD** crashes, or returns unexpected data or behaviour, [CBSD command debuging](https://github.com/cbsd/cbsd/blob/develop/share/docs/general/cmdsyntax_cbsd.md) can be enabled. 

```sh
env CBSD_DEBUG=1 cbsd node mode=add node=192.168.1.222 pw=very_strong_plain_password port=22 
```

```sh
env CBSD_DEBUG=1 cbsd jls
```

If the bug is reproducible, and an actual bug is discovered, please report the issue via e-mail: **CBSD** *at* **bsdstore.ru**, or better yet submit a pull request that describes the issue and contains the code to resolve the issue.