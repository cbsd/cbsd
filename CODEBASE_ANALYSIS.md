# CBSD Codebase Analysis Report

This report provides an overview of the CBSD project structure, its core components, and its documentation system to assist in updating the project's documentation.

## Project Overview

CBSD is a management layer for FreeBSD jails, bhyve, Xen, and QEMU. It aims to provide a unified interface (both CLI and TUI) for managing various virtualization and containerization technologies.

## Core Architecture

### 1. The Custom Shell (`cbsdsh`)
The heart of CBSD is a customized shell located in `bin/cbsdsh`. This shell is based on the FreeBSD shell (`sh`) but includes several built-in commands tailored for CBSD's needs, such as:
- `sqlcmd`: Integrates SQLite support directly into the shell.
- `jail`: Enhanced jail management built-ins.
- `cbsdlogtail`, `cbsd_fwatch`, etc.

Most CBSD scripts use the shebang `#!/usr/local/bin/cbsd`, which typically points to this custom shell.

### 2. Control Scripts
The high-level logic for different subsystems is organized into separate directories:
- `jailctl/`: Scripts for managing jails (e.g., `jstart`, `jstop`, `jls`).
- `bhyvectl/`: Scripts for managing bhyve virtual machines.
- `nodectl/`: Scripts for managing nodes in a CBSD cluster.
- `qemuctl/`, `vboxctl/`, `xenctl/`: Scripts for other hypervisors.

### 3. Subroutines (`subr/`)
The `subr/` directory contains a large collection of shell snippets (`.subr` files) that provide shared functionality across the control scripts. Significant files include:
- `zfs.subr`: ZFS-related operations.
- `bhyve.subr`: Core bhyve logic.
- `jail.subr`: Core jail logic.
- `nc.subr`: Networking and core utilities.

### 4. Configuration and Defaults (`etc/defaults/`)
CBSD relies heavily on default configuration files for various operating systems and versions. These are stored in `etc/defaults/` and include:
- `jail-freebsd-default.conf`: Default settings for FreeBSD jails.
- `vm-linux-*`: Cloud-init and VM configuration templates for numerous Linux distributions.
- `global.conf`: Global CBSD settings.

## Documentation System

### 1. READMEs
- `README.md`: The primary entry point, providing project history, goals, and high-level usage.
- `README.DragonflyBSD.md`, `README.HardenedBSD.md`, `README.OpenBSD.md`: Platform-specific notes.

### 2. Documentation Fragments (`share/docs/`)
A significant portion of the documentation is stored as markdown fragments in `share/docs/`. These are often organized by subsystem:
- `share/docs/jail/`: Documentation for jail-related workflows. Many files follow the `wf_*_ssi.md` naming convention (WorkFlow, Server Side Include).
- `share/docs/bhyve/`: Documentation for bhyve.
- `share/docs/general/`: General guides and quickstarts.

### 3. Manual Pages (`man/`)
The project currently has a limited number of manual pages in the `man/` directory (e.g., `cbsd.8`, `cbsd-jls.8`). Expanding this is a potential area for contribution.

## Internal Tools and Utilities
- `bin/src/`: C source code for some core utilities (e.g., `cfetch`, `cbsdsftp`).
- `misc/src/`: Various C-based helper tools like `sipcalc`, `sqlcli`, and `daemonize`.
- `tools/src/`: Statistics and monitoring tools (e.g., `racct-jail-statsd`).

## Development Workflow
- **Makefile**: The project uses a standard BSD Makefile for building the custom shell and various C helpers.
- **Tests**: A test suite is available in the `tests/` directory and can be run via `./runall`.

## Recommendations for Documentation Updates
1. **Consistency**: Ensure that the `wf_*_ssi.md` fragments are up-to-date with the latest script changes in `jailctl/` and `bhyvectl/`.
2. **Expansion of Man Pages**: Many commands in `jailctl/` lack corresponding manual pages.
3. **Configuration Reference**: The vast array of files in `etc/defaults/` could be better documented, perhaps with a generated reference guide.
