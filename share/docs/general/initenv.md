# CBSD Initialization

To use **CBSD**, you must first initialize its working directory. This process sets up the necessary directory hierarchy, configuration files, and system integration (such as `sudo` permissions and `rc.conf` entries).

## Quick Start (Interactive Mode)

The most common way to initialize **CBSD** is via the interactive dialogue. By default, **CBSD** uses `/usr/jails` as its working directory.

To start the initialization, run:

```bash
env workdir="/usr/jails" cbsd initenv
```

> [!NOTE]
> Setting the `workdir` environment variable is only required during the **first** initialization. Once initialized, the path is stored in `/etc/rc.conf` and **CBSD** will use it automatically for subsequent runs.

### ZFS Recommendation

If you are using a ZFS-based platform, it is highly recommended to create a separate dataset for the **CBSD** working directory *before* running `initenv`. This avoids conflicts with host-level snapshot systems and simplifies migration.

```bash
# Example: Creating a dedicated dataset
zfs create -o mountpoint=/usr/jails zroot/jails
```

### Configuration Parameters

During the interactive setup, you will be asked a series of questions. Pressing **Enter** without a value will accept the system default. Key settings include:

- **nodename**: A unique name for this node (e.g., `node1.my.domain`).
- **nodeip**: The static IP address used for node interconnection.
- **jnameserver**: DNS servers for new jails (e.g., `8.8.8.8, 1.1.1.1`).
- **nodeippool**: List of subnets for jail IP allocation (e.g., `10.0.0.0/24`).
- **nat_enable**: Enable NAT for private (RFC1918) networks.
- **zfsfeat**: Whether to use ZFS features like clones and snapshots.

---

## Automated Setup (Preseed Mode)

For automated deployments or repeated setups, you can provide an answer file (preseed) to `initenv`. This allows for non-interactive initialization.

### Using a Configuration File

You can use the sample configuration at `/usr/local/cbsd/share/initenv.conf` as a template. To run initialization with a preseed file:

```bash
cbsd initenv /path/to/your/initenv.conf
```

### Overriding Parameters

You can also override specific parameters via command-line arguments:

```bash
cbsd initenv /usr/local/cbsd/share/initenv.conf default_vs=1 inter=0
```

> [!TIP]
> Use `inter=0` to ensure the process is completely silent and does not prompt for any user input.

---

## Post-Initialization & Updates

Once initialized, you can modify settings at any time using the TUI:

```bash
cbsd initenv-tui
```

### Updating CBSD

**IMPORTANT:** You must run `cbsd initenv` after every **CBSD** update (e.g., via `pkg upgrade`). This ensures that your working directory is migrated to the latest version and all internal scripts are updated.

```bash
cbsd initenv
```

---

## Initialization Hooks

You can extend the initialization process by placing custom scripts in the `${workdir}/upgrade` directory. These scripts are executed during every `initenv` run.

- **Pre-hooks**: Scripts starting with `pre-initenv-` (runs before modifications).
- **Post-hooks**: Scripts starting with `post-initenv-` (runs after successful update).

These hooks are useful for automated backups, external notifications, or custom system tuning. Ensure your scripts have the executable flag set:

```bash
mkdir -p ~cbsd/upgrade
# Add your scripts here
```
