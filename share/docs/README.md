# CBSD Documentation

The CBSD book doesn't exist yet, but thanks to *Michael Reim*, this series of articles could be a good start: [Exploring the CBSD virtual environment management framework](https://eerielinux.wordpress.com/2022/12/10/exploring-the-cbsd-virtual-environment-management-framework-part-1-introduction-and-installation/)

## General information

- [CBSD Quick Start](general/cbsd_quickstart.md)
- [What you need to know about CBSD](general/cbsd_additional.md)
- [cbsd syntax](general/cmdsyntax_cbsd.md)
- [About CBSD shell and SQLite3](general/cbsdsh.md) 
- [CBSD GUI interface](general/cbsd_gui.md)
- [Working with CBSDfile](general/cbsdfile.md) ( up, destroy, login, exec )
- [Building and upgrading bases](general/base_cbsd.md)
- [CBSD integration with PHPIPAM (IP management)](general/wf_ipam.md)
- [Tags and custom facts](general/tag_n_facts.md) ( jget/bget/xget/jls/bls/bget.. )
- [API module: private cloud via API](general/cbsd_api.md)
- [CBSD and OCI containers](general/cbsd_oci.md)
- [CBSD mirrors](https://github.com/cbsd/mirrors)

## Virtual environments

<!--
- [CBSD init](general/initenv.md) :: initenv

- [jail settings](cbsd_rcconf.md)
- [cbsd jail and VIMAGE (vnet)](cbsd_vnet.md)
- [cbsd and Qemu user-mode](cbsd_qemu_usermode.md)
- [CBSD and Linux jails](cbsd_linux_jails.md)
- [encrypting images and CBSD](cbsd_geli.md) ( **GELI**, native **ZFS** encryption )
- [CBSD environment variables](/wf_cbsd_variables_ssi.md) :: external hooks variables
- [Operation with repository](/wf_repo_ssi.md) :: repo, repo-tui
- [How does a helper for CBSD image](/wf_imghelper_ssi.md) :: imghelper
- [cbsd command history](/wf_history_ssi.md) :: CBSD history
- [Modification which are carried out by CBSD scripts in FreeBSD](custom_freecbsd.md)
- [cbsd taskd](cbsd_taskd.md) :: task, taskls
- [FreeBSD: Xorg in jail](xorg_in_jail.md)
- [FreeBSD: CBSD and bhyve](bhyve.md)
- [FreeBSD: CBSD and XEN](xen.md)
- [CBSD syslog and debbuging](syslog.md)
- [CBSD RACCT statistics](racct_exports.md)
- [Broker driven CBSD cluster (example)](/broker_driven_sample_ssi.md)
- [API module: private cloud via API](/cbsd_api_ssi.md)
- [VPC with CBSD (vxlan)](/wf_vpc_ssi.md)
- [CBSD integration with PHPIPAM (IP management)](/wf_ipam_ssi.md)
- [CBSD integration with MONIT (health-check)](/wf_monit_ssi.md)
--->
- [CBSD + bhyve handbook](bhyve/handbook.md) (platform: FreeBSD)
- [CBSD + jail handbook](jail/handbook.md) (platform: FreeBSD, DragonflyBSD)
- [CBSD + QEMU handbook ](qemu/handbook.md) (platform: FreeBSD, Linux)
- [CBSD + XEN handbook ](xen/handbook.md) (platform: FreeBSD)

## Help

Most of the documentation is embedded directly into the scripts. You can access it using the help argument:

```
cbsd <cmd> --help
```

To get a list of all commands or for a specific module:

```
cbsd help
cbsd help module=bhyve
```

There is also a community chat on Telegram:

* English: [@cbsdofficial](https://t.me/cbsdofficial)

* Russian: [@cbsdofficial_ru](https://t.me/cbsdofficial_ru)

