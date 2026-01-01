# Building and Upgrading Bases

In **CBSD**, a "base" refers to a directory containing a full copy/hierarchy of FreeBSD (or other supported OS) files used as the template for jails.

Bases are typically stored in `${workdir}/basejail/` and follow the naming convention: `base_${arch}_${target_arch}_${ver}`.

## Base Acquisition Methods

When creating a container or if base files are missing, **CBSD** offers four primary methods for obtaining a base.

| Method | Speed | Requirement | Use Case |
| :--- | :--- | :--- | :--- |
| **`repo`** | Fast | Internet/Network | Standard user. Downloads pre-compiled `.txz` archives. |
| **`populate`** | Very Fast | Local Match | Host matches target version. Clones files from the host environment. |
| **`extract`** | Fast | Local Archive | Offline install. Unpacks an existing `.txz` file from a provided path. |
| **`build`** | Slow | Sources & CPU | Advanced user. Downloads sources via Git and compiles from scratch. |

### Viewing Registered Bases

To list all bases currently registered in your **CBSD** environment:

```bash
cbsd bases
```

---

## Configuration of Base Acquisition

Starting with **CBSD** 12.0.4, you can adjust the default base acquisition method and customize sources via configuration files.

The primary configuration file is `~cbsd/etc/defaults/FreeBSD-bases.conf` (or `HardenedBSD-bases.conf`). To customize settings, copy this file to `~cbsd/etc/FreeBSD-bases.conf` and modify it.

Available variables include:
- `auto_baseupdate`: Automatically update the base to the latest patch level (e.g., via `freebsd-update`).
- `default_obtain_base_method`: List of methods to try (e.g., `"extract repo"`).
- `default_obtain_base_extract_source`: Local path to search for archives.
- `default_obtain_base_repo_sources`: List of remote URLs for archives.

Example customization:
```bash
auto_baseupdate=1
default_obtain_base_method="extract repo"
default_obtain_base_extract_source="/nfs/bases/${platform}/base-${arch}-${ver}.txz"
```

---

## 1. Network Acquisition (`repo`)

This is the default and most common method. **CBSD** fetches pre-compiled archives from official repositories or **CBSD** mirrors.

- Set `fbsdrepo=1` in `initenv-tui` to use **ONLY** official FreeBSD repositories.
- Use `inter=0` for non-interactive execution, ideal for automation.

---

## 2. Local Population (`populate`)

If you are creating a jail of the same architecture and version as your host system, **CBSD** can "populate" the base by copying existing files from your host. This avoids network traffic and compilation.

---

## 3. Manual Extraction (`extract`)

If you have a local `base.txz` archive, you can point **CBSD** to it during initialization or when prompted.

---

## 4. Building from Source (`build`)

For maximum control or to track specific branches, you can compile the base yourself.

### Step A: Fetch Sources (`srcup`)

Fetch the FreeBSD source tree into `${workdir}/src/src_${ver}`. **CBSD** uses Git by default (previously SVN).

```bash
cbsd srcup [ver=XX] [rev=XX] [stable=0|1]
```

**Common Parameters:**
- `ver`: Specify version (e.g., `14.1`, `head` for CURRENT).
- `stable=1`: Fetch RELENG branch (e.g., `ver=14 stable=1` for RELENG_14).
- `rev`: Specify a particular Git/SVN revision.

### Step B: Compile (`buildworld`)

Compile the source code into objective files.

```bash
cbsd buildworld [ver=XX] [arch=XX] [clean=1] [maxjobs=XX]
```

**Common Parameters:**
- `clean=1`: Clean old object files before building (`make clean`).
- `maxjobs`: Number of parallel jobs. Defaults to CPU core count.
- `basename`: Build into a custom directory name (results in `base_${basename}_${arch}_${ver}`).

### Step C: Install (`installworld`)

Install the compiled world into the `${workdir}/basejail` directory.

```bash
cbsd installworld [ver=XX] [arch=XX] [basename=XX]
```

> [!TIP]
> Use **`cbsd world`** to run both `buildworld` and `installworld` sequentially.

---

## Advanced Customization

### Alternative Basenames
You can maintain multiple base variants using the `basename` parameter.
```bash
cbsd world ver=14.1 basename=lite
```

### Custom `src.conf`
Place a custom [src.conf(5)](https://man.freebsd.org/src.conf/5) in `${workdir}/etc/` named `src-${basename}.conf` to apply specific build knobs.

---

## Deleting a Base

Use **`cbsd removebase`** to delete a registered base and its files.

```bash
cbsd removebase [ver=XX] [arch=XX] [stable=0|1] [target_arch=XX]
```

If no arguments are provided, it defaults to the host's version and architecture.
