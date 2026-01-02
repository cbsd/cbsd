# CBSD Documentation

The CBSD book doesn't exist yet, but thanks to *Michael Reim*, this series of articles could be a good start: [Exploring the CBSD virtual environment management framework](https://eerielinux.wordpress.com/2022/12/10/exploring-the-cbsd-virtual-environment-management-framework-part-1-introduction-and-installation/)

## General Information

- [CBSD Quick Start](general/cbsd_quickstart.md)
- [What You Need to Know About CBSD](general/cbsd_additional.md)
- [CBSD Syntax](general/cmdsyntax_cbsd.md)
- [Jail Settings](jail/cbsd_rcconf.md)
- [About CBSD Shell and SQLite3](general/cbsdsh.md) 
- [CBSD GUI Interface](general/cbsd_gui.md)
- [Working with CBSDfile](general/cbsdfile.md) ( up, destroy, login, exec )
- [Building and Upgrading Bases](general/base_cbsd.md)
- [CBSD Integration with PHPIPAM (IP Management)](general/wf_ipam.md)
- [Tags and Custom Facts](general/tag_n_facts.md) ( jget/bget/xget/jls/bls/bget.. )
- [API Module: Private Cloud via API](general/cbsd_api.md)
- [CBSD and OCI containers](general/cbsd_oci.md)
- [CBSD Mirrors](https://github.com/cbsd/mirrors)

## Virtual Environments

<!--
- [CBSD init](general/initenv.md) :: initenv


- [CBSD Jail and VIMAGE (vnet)](cbsd_vnet.md)
- [CBSD and Qemu User-Mode](cbsd_qemu_usermode.md)
- [CBSD and Linux Jails](cbsd_linux_jails.md)
- [Encrypting Images and CBSD](cbsd_geli.md) ( **GELI**, native **ZFS** encryption )
- [CBSD Environment Variables](/wf_cbsd_variables_ssi.md) :: external hooks variables
- [Operation with Repository](/wf_repo_ssi.md) :: repo, repo-tui
- [How does a helper for CBSD image](/wf_imghelper_ssi.md) :: imghelper
- [CBSD Command History](/wf_history_ssi.md) :: CBSD history
- [Modification Which Are Carried Out by CBSD Scripts in FreeBSD](custom_freecbsd.md)
- [CBSD Taskd](cbsd_taskd.md) :: task, taskls
- [FreeBSD: Xorg in Jail](xorg_in_jail.md)
- [FreeBSD: CBSD and bhyve](bhyve.md)
- [FreeBSD: CBSD and XEN](xen.md)
- [CBSD Syslog and Debugging](syslog.md)
- [CBSD RACCT Statistics](racct_exports.md)
- [Broker Driven CBSD Cluster (Example)](/broker_driven_sample_ssi.md)
- [API Module: Private Cloud via API](/cbsd_api_ssi.md)
- [VPC with CBSD (vxlan)](/wf_vpc_ssi.md)
- [CBSD Integration with PHPIPAM (IP Management)](/wf_ipam_ssi.md)
- [CBSD Integration with MONIT (Health-Check)](/wf_monit_ssi.md)
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

