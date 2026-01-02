# What You Need to Know about CBSD

This guide provides a foundational understanding of CBSD, guiding you through its core concepts, architectural layout, and day-to-day operations.

> [!NOTE]
> Throughout this guide, we use `/usr/jails` as the default working directory. This path is represented by the internal variable `$workdir`. If you initialized CBSD in a different location, please substitute `/usr/jails` with your specific path.

## Contents
- [Phase 1: Concepts & Core Architecture](#phase-1-concepts--core-architecture)
- [Phase 2: Project Layout & Data Management](#phase-2-project-layout--data-management)
- [Phase 3: Working with CBSD](#phase-3-working-with-cbsd)
- [Phase 4: Advanced Workflow & Operations](#phase-4-advanced-workflow--operations)
- [Phase 5: Maintenance & Support](#phase-5-maintenance--support)

---

## Phase 1: Concepts & Core Architecture

### Introduction

**CBSD** is a powerful abstraction layer designed for the [FreeBSD](https://www.freebsd.org/) ecosystem. It simplifies the management of [jail(8)](http://www.freebsd.org/cgi/man.cgi?query=jail&sektion=8), the [bhyve hypervisor](http://www.freebsd.org/cgi/man.cgi?query=bhyve&sektion=8), and the [XEN project hypervisor](http://www.xenproject.org/).

CBSD leverages native FreeBSD subsystems to create a comprehensive container and VM management system:
- **Networking**: [vnet (VIMAGE)](https://klarasystems.com/articles/virtualize-your-network-on-freebsd-with-vnet/), [vale](https://man.freebsd.org/cgi/man.cgi?query=vale&sektion=4&manpath=FreeBSD+12.0-RELEASE+and+Ports), [vxlan](https://wiki.freebsd.org/vxlan), [carp](https://docs.freebsd.org/en/books/handbook/advanced-networking/)
- **Storage**: [ZFS](https://docs.freebsd.org/en/books/handbook/zfs/), [hastd](https://wiki.freebsd.org/HighlyAvailableStorage)
- **Security & Limits**: [racct/rctl](https://klarasystems.com/articles/controlling-resource-limits-with-rctl-in-freebsd/), [pf/ipfw/ipfilter](https://docs.freebsd.org/en/books/handbook/firewalls/)

Whether you are managing a single standalone workstation or a large-scale cloud cluster, CBSD provides a unified API and user-friendly tools (like `bsdconfig`-style dialogs) to streamline your infrastructure.

### Core Terminology

- **Node (host):** A physical server hosting the virtual environment.
- **Jail (guest):** A lightweight, isolated OS-level virtualization environment.
- **Cloud/Farm:** A cluster of interconnected nodes functioning as a peer network.
- **Base:** A copy of the FreeBSD base system files used as a template for guests.
- **$workdir:** The CBSD working directory (default: `/usr/jails`), initialized via `cbsd initenv`.
- **$jname:** The unique name assigned to a jail or VM.

---

## Phase 2: Project Layout & Data Management

### Filesystem Hierarchy

CBSD's data is primarily stored within `/usr/jails`. Below is the summary of the hierarchy:

| Directory Path | Description |
| :--- | :--- |
| `/usr/jails/.ssh` / `.rssh` | Private and public keys for local and remote node authentication. |
| `/usr/jails/basejail` | Storage for FreeBSD bases and kernels (templates). |
| `/usr/jails/etc` | CBSD configuration files (overrides). |
| `/usr/jails/jails-data` | **Critical**: The persistent data for all your jails. |
| `/usr/jails/jails-fstab` | Custom filesystem mount points for each jail. |
| `/usr/jails/var/db` | SQLite databases containing the inventory and state of all guests. |
| `/usr/local/cbsd` | The static CBSD binary and script files (the "engine"). |

### Base Types: `baserw=0` vs `baserw=1`

The way a jail handles its base system is a critical architectural choice:

1. **Read-Only Base (`baserw=0`)**:
   - The jail mounts a shared base system via **nullfs**.
   - **Advantage**: Security (the base cannot be modified) and simplified updates (updating one template updates all associated jails).
   - **Storage**: Located in `/usr/jails/jails/$jname`.

2. **Read-Write Base (`baserw=1`)**:
   - The jail has its own dedicated copy of the base system.
   - **Advantage**: Full flexibility to modify core system files within the jail.
   - **Storage**: Located in `/usr/jails/jails-data/$jname`.

### SQLite3 Inventory
All jail settings are stored in an SQLite3 database, typically pointed to by `/usr/jails/var/db/local.sqlite`. You can query this directly:
```sh
sqlite3 /usr/jails/var/db/local.sqlite "SELECT jname, ip4_addr FROM jails;"
```

---

## Phase 3: Working with CBSD

### Configuration & Customization
CBSD follows the "FreeBSD way" for configuration. Default settings are found in `/usr/jails/etc/defaults/`. To override a setting, create a file with the same name in `/usr/jails/etc/`.

#### Visual Feedback (ANSI Color)
By default, CBSD uses colorized output. If this interferes with scripts or personal preference, disable it using:
```sh
env NOCOLOR=1 cbsd jls
```

### Extending with Modules
CBSD's functionality can be extended via modules located in `/usr/local/cbsd/modules`.
1. Add the module name to `~workdir/modules.conf`.
2. Re-initialize: `cbsd initenv`.
*Example: The **ClonOS** project uses this to provide a web-based GUI and VNC terminals.*

---

## Phase 4: Advanced Workflow & Operations

### Batch Operations (Wildcards)
Most CBSD commands support the `jname` parameter with wildcard expansion (`*`):
```sh
cbsd jstart jname='web*'   # Starts all jails beginning with 'web'
cbsd jstop jname='*'       # Stops all jails
```

### Networking & Port Forwarding
The `expose` command manages TCP/UDP port forwarding from the host to a jail using `ipfw`:
```sh
cbsd expose jname=test2 mode=add in=80 out=80
```
CBSD also includes built-in traffic counting in the `99-2000` rule range of `ipfw`.

### Backups
To secure your environment, ensure you back up the following directories:
- `/usr/jails/var/db` (Metadata)
- `/usr/jails/jails-fstab` (Mount configurations)
- `/usr/jails/jails-system` (Scripts/Stats)
- `/usr/jails/jails-data` (Application data)

---

## Phase 5: Maintenance & Support

### Troubleshooting (Debug Mode)
If you encounter unexpected behavior, run commands with `CBSD_DEBUG=1` to enable detailed tracing:
```sh
env CBSD_DEBUG=1 cbsd jls
```

### Encountering Problems
If you find a bug:
1. Enable debug mode to capture the trace.
2. Report the issue to **cbsd@bsdstore.ru** or submit a pull request on GitHub.

For more detailed technical specs, see the [CBSD Customization Guide](https://github.com/cbsd/cbsd/blob/develop/share/docs/general/custom_freecbsd.md).