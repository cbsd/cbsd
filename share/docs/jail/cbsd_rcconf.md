# Jail Settings (rc.conf)

Each jail managed by CBSD has its own configuration settings used during the start and stop procedures. These settings are primarily defined during jail creation but can be modified later using the configuration tool:

```sh
cbsd jconfig jname=myjail
```

> [!NOTE]
> Most changes require the jail to be restarted to take effect. This guide uses `/usr/jails` as the default working directory (`$workdir`).

---

## 1. Guest Identifiers

These settings define the core identity and network presence of the jail.

### `jname`
The unique internal name of the jail.
- **Example**: `jname="jail1"`
- **Note**: This cannot be changed manually. Use `cbsd jrename` to rename a jail properly, as this name is linked to multiple directories and metadata tables.

### `host_hostname`
The Fully Qualified Domain Name (FQDN) of the jail.
- **Example**: `host_hostname="jail1.my.domain"`
- **Note**: Use `cbsd jrename` to change this.

### `path`
The directory used as the root (`/`) when the jail is running.
- **Example**: `path="/usr/jails/jails/jail1"`
- **Baserw=0**: CBSD mounts a read-only base template into this path and then overlays the jail's unique data.
- **Baserw=1**: The jail starts directly from its data directory (usually `/usr/jails/jails-data/jail1-data`).

### `ip4_addr`
The IP address assigned to the jail.
- **Single IP**: `ip4_addr="10.0.0.5/24"`
- **Multiple IPs**: `ip4_addr="10.0.0.5/24,192.168.0.2/30"`

---

## 2. Storage & Base Management

These settings control how the jail interacts with the FreeBSD base system and host directories.

### `baserw`
Determines if the jail has a private, writable base system.
- **`0` (Read-Only)**: Secure and efficient. Multiple jails share one read-only base template via `nullfs`.
- **`1` (Read-Write)**: Each jail has its own full copy of the FreeBSD base in its data directory.

### `ver` and `arch`
Specifies the version and architecture of the FreeBSD base used by the jail.
- **Example**: `ver="14.1"`, `arch="amd64"`
- **Note**: If `baserw=0`, CBSD looks for the template in `/usr/jails/basejail/base_amd64_14.1`.

### `basename`
Allows the use of a customized base template (e.g., a "lite" or "hardened" build).
- **Example**: `basename="lite"` (CBSD will look for `base_lite_amd64_...`)

### Extra Mounts
Automatically mount host source trees or ports into the jail in read-only mode:
- **`mount_src="1"`**: Mounts `/usr/src`.
- **`mount_obj="1"`**: Mounts `/usr/obj`.
- **`mount_kernel="1"`**: Mounts `/boot/kernel` (useful for DTrace).
- **`mount_ports="1"`**: Mounts the host's `/usr/ports` tree.

---

## 3. Network Configuration

### `interface`
Controls which host interface the jail's IP is bound to.
- **`auto`**: CBSD automatically selects the interface based on the jail's subnet.
- **`igb0`**: Force binding to a specific interface.
- **`""` (Empty)**: CBSD will not manage the IP. Useful if the IP is already manually configured on the host.

### `vnet`
Enables the Virtual Network Stack (VIMAGE), providing the jail with its own independent network stack.
- **`1`**: Enabled.
- **`0`**: Disabled (shares the host network stack).

### `setfib`
Assigns the jail to a specific routing table (FIB).
- **Example**: `setfib="1"`
- **Note**: Requires `net.fibs` to be configured in `/boot/loader.conf`.

### `floatresolv`
- **`1`**: CBSD automatically manages `/etc/resolv.conf`, pointing the jail to the host or cluster name servers.

---

## 4. Mounts & Permissions

### `mount_devfs`
- **`1`**: Mounts the device file system into `/dev`. Required by most services.

### `mount_fstab`
Path to the jail-specific fstab file for custom mounts.
- **Default**: `/usr/jails/jails-fstab/fstab.jail1`

### Security Flags
- **`allow_mount="1"`**: Allows the jail to mount file systems.
- **`allow_devfs="1"`**: Allows mounting devfs inside the jail.
- **`allow_nullfs="1"`**: Allows mounting nullfs inside the jail.

---

## 5. Resource Control

### `cpuset`
Binds the jail to specific CPU cores.
- **`0`**: No binding (uses all cores).
- **`1`**: Bind to the first core only.
- **`0-3`**: Bind to cores 0 through 3.

### `devfs_ruleset`
The ID of the devfs ruleset to apply to the jail's `/dev`.
- **Default**: `4` (standard jail ruleset in `/etc/devfs.rules`).

---

## 6. Boot & Scripting

### `astart`
- **`1`**: Automatically start the jail when the host system boots.
- **`0`**: Manual start only.

### `exec_start` / `exec_stop`
Custom commands to execute when starting or stopping the jail.
- **Start**: `exec_start="/bin/sh /etc/rc"`
- **Stop**: `exec_stop="/bin/sh /etc/rc.shutdown"`

### `applytpl`
- **`1`**: Automatically apply CBSD configuration templates (e.g., `pkg.conf`, `hosts`) to the jail's internals.

### `exec.consolelog`
Directs the output of the jail's start/stop scripts to a file.
- **`0`**: Display output directly to the terminal.
- **`1`**: Log to `/usr/jails/var/log/$jname.log`.
- **`/dev/null`**: Discard all output.

---
Copyright © 2013—2025 CBSD Team.
