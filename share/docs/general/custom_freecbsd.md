# System Modifications for CBSD Integration

To provide a fully integrated and automated environment, **CBSD** makes (or suggests) a series of configuration changes to the FreeBSD host. This page explains what these changes are, why they are necessary, and how to manage them.

> [!TIP]
> If you plan to uninstall **CBSD**, remember to revert these changes to return your system to its original state.

---

## Host Configuration (`/etc/rc.conf`)

The following parameters in `/etc/rc.conf` are modified to ensure **CBSD** operates correctly.

### 1. `rcshutdown_timeout`

This parameter defines the maximum time the system waits for services to stop during shutdown. The default FreeBSD value (120 seconds) is often insufficient for hosts running many jails or VMs, especially those running databases or other sensitive services that require graceful termination.

**CBSD** increases this value during the first initialization. If you prefer to keep your existing value, ensure it is explicitly set in `/etc/rc.conf` before running `cbsd initenv`. **CBSD** will not overwrite an existing entry.

```bash
# To prevent CBSD from suggesting a change, you can pre-set the default:
grep rcshutdown_timeout /etc/defaults/rc.conf >> /etc/rc.conf
```

### 2. SSH Daemon Configuration (`sshd_flags`)

By default, **CBSD** suggests moving the host's SSH service to port **22222**.

```bash
sshd_flags="-oPort=22222"
```

#### Why change the port?
- **Avoid Conflicts**: Prevents port conflicts between the host and jails sharing the same IP address.
- **Node Identification**: In a multi-node environment, port 22222 consistently identifies the physical host (master node), while port 22 (or 2222) can be reserved for guest environments.
- **Brute-force Reduction**: While not a primary security measure, it reduces the volume of automated scanners targeting port 22.

#### Customizing SSH Behavior
If you want to listen on both the standard port and the CBSD port, use:
```bash
sshd_flags="-oPort=22 -oPort=22222 -oUseDNS=no"
```

#### Client Configuration
To simplify connections, we recommend using a `~/.ssh/config` file:
```text
# Standard server
Host otherserver.my.domain
    Port 22
    User root

# Template for CBSD Jails
Host *.j.*
    Port 2222

# Default for CBSD Nodes
Host *
    Port 22222
    ControlMaster auto
    ControlPath ~/.ssh/sockets/%r@%h:%p
```

### 3. Device Rulesets (`devfs_load_rulesets`)

Jails use `devfs` rulesets to restrict access to hardware devices. By default, **CBSD** jails use ruleset **4**. To enable these rulesets at boot, `devfs_load_rulesets="YES"` is required. Without this, jails might have access to all system devices, which is a security risk.

### 4. CBSD Service Settings

The initialization script (`cbsd initenv`) adds the following system-level variables:

```bash
cbsdd_enable="YES"
cbsd_workdir="/usr/jails"
cbsdrsyncd_enable="YES"
cbsdrsyncd_flags="--config=/usr/jails/etc/rsyncd.conf"
```

- **cbsdd**: The main management daemon.
- **cbsdrsyncd**: Runs an internal `rsync` service on port **1873** for jail migrations (`cbsd jcoldmigrate`). If you don't need migration features, you can set this to `NO`.

---

## Kernel & Boot Configuration (`/boot/loader.conf`)

If you enable NAT during initialization, **CBSD** may add kernel module requirements to your loader configuration:

```bash
# Common modules loaded for NAT and Firewalling
pf_load="YES"
ipfw_load="YES"
ipfw_nat_load="YES"
libalias_load="YES"
net.inet.ip.fw.default_to_accept="1"
```

> [!IMPORTANT]
> `net.inet.ip.fw.default_to_accept="1"` is added as a safety measure. By default, IPFW blocks all traffic. This setting ensures you don't lose access to the server if your firewall rules are misconfigured. It adds a final `allow ip from any to any` rule.

### Networking Forwarding
When NAT is enabled (`cbsd naton`), the system automatically enables IP forwarding:
```bash
sysctl net.inet.ip.forwarding=1
sysctl net.inet6.ip6.forwarding=1
```

---

## Privileged Access (`sudoers.d`)

Most **CBSD** operations require root privileges. To allow the `cbsd` user to manage the system without constant password prompts, a specific `sudoers` configuration is created:

```text
# /usr/local/etc/sudoers.d/cbsd_sudoers
Defaults     env_keep += "workdir DIALOG NOCOLOR"
Cmnd_Alias   CBSD_CMD = /usr/jails/sudoexec/*, /usr/local/cbsd/sudoexec/*
cbsd         ALL=(ALL) NOPASSWD: CBSD_CMD
```

This allows the `cbsd` user to execute scripts in the `sudoexec` directories with root authority.

---

## Network Traffic Counting (IPFW)

If IPFW is enabled, **CBSD** uses it to track bandwidth usage for jails. It reserves a range of rule numbers (defined in `$workdir/cbsd.conf`) for these counters.

```bash
fwcount_st = "99"
fwcount_end = "2000"
```

If you are writing custom IPFW rules, avoid using numbers in this range (99–2000) as **CBSD** may overwrite them when jails start or stop.

---

Copyright © 2013—2024 CBSD Team.
