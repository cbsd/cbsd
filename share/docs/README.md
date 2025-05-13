# CBSD Documentation

The CBSD book doesn't exist yet, but thanks to *Michael Reim*, this series of articles could be a good start: [Exploring the CBSD virtual environment management framework](https://eerielinux.wordpress.com/2022/12/10/exploring-the-cbsd-virtual-environment-management-framework-part-1-introduction-and-installation/)

## General information

- [CBSD Quick Start](general/cbsd_quickstart.md)
- [CBSD GUI interface](general/cbsd_gui.md)
- [CBSD Built-in help](general/cbsd_help.md) ( help )
- [CBSD Upgrading](general/cbsd_upgrading.md) ( initenv )
- [CBSD Config/Customizing](general/cbsd_config.md) ( initenv, configuration files/hier )
- [Working with NAT](general/cbsd_nat.md) ( natcfg, naton, natoff )
- [Working with CBSDfile](general/cbsdfile.md) ( up, destroy, login, exec )
- [Broker driven CBSD cluster (example)](general/)
- [CBSD integration with PHPIPAM (IP management)](general/wf_ipam.md)
- [Tags and custom facts](general/tag_n_facts.md) ( jget/bget/xget/jls/bls/bget.. )
- [API module: private cloud via API](general/cbsd_api.md)
- [CBSD and OCI containers](general/cbsd_oci.md)
- [About fetch work: CBSD mirrors](https://github.com/cbsd/mirrors)


<!---
- [What you need to know about CBSD](general/cbsd_additional.md)
- [About CBSD shell and SQLite3](general/cbsdsh.md)
- [CBSD init](general/initenv.md) :: initenv
- [Building and upgrading bases](base_cbsd.md) :: buildworld, installworld, world, bases, removebase, upgrade
- [cbsd syntax](cmdsyntax_cbsd.md)
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

## Container: Operation with jail

[Single file](/workflow_cbsd.md)

- [jail create via dialog menu](jail/wf_jcreate_ssi.md) :: jcreate, jconstruct-tui
- [other methods of creating jail](jail/wf_jcreate_secondary_ssi.md) :: jcreate, jconstruct
- [Profiles for jail creation](jail/wf_jprofiles_ssi.md) :: jcreate, jconstruct-tui
- [jail config](jail/wf_jconfig_ssi.md) :: jconfig
- [starting and stoping jail](jail/wf_jstop_jstart_ssi.md) :: jstart, jstop, jrestart
- [jail starting order](jail/wf_jorder_ssi.md) :: jorder
- [jail removal](jail/wf_jremove_ssi.md) :: jremove
- [jail renaming](jail/wf_jrename_ssi.md) :: jrename
- [jail upgrade, p1: bases](jail/wf_jupgrade_ssi.md) :: jupgrade
- [jail upgrade, p2: etcupdate](jail/wf_etcupdate_ssi.md) :: etcupdate
- [jails list](jail/wf_jls_ssi.md) :: jls
- [command execution in jail](jail/wf_jexec_ssi.md) :: jexec
- [jail login](jail/wf_jlogin_ssi.md) :: jlogin
- [work with jail parameters](jail/wf_jget_ssi.md) :: jset,jget
- [jail cloning](jail/wf_jclone_ssi.md) :: j\[r\]clone
- [jails snapshot (zfs-only)](jail/wf_jsnapshot_ssi.md) :: jsnapshot
- [jail export](jail/wf_jexport_ssi.md) :: jexport
- [jail import](jail/wf_jimport_ssi.md) :: jimport
- [backup and file replication for jail](jail/wf_jbackup_ssi.md) :: jbackup
- [jail description](jail/wf_jdescr_ssi.md) :: jdescr
- [jail cold migration](jail/wf_jcoldmigrate_ssi.md) :: jcoldmigrate
- [jail limits control](jail/wf_jrctl_ssi.md) :: jrctl, jrctl-tui
- [Port forwarding for jail](jail/wf_expose_ssi.md) :: expose
- [Generation of bootable ISO and USB Memstick from jail](jail/wf_jail2iso_ssi.md) :: jail2iso
- [Searching for jail in node farm](jail/wf_jlocate_ssi.md) :: jwhereis, jailmapdb
- [Copying files from/to jail filesystem](jail/wf_jailscp_ssi.md) :: jailscp
- [A few words about jail traffic counting](jail/wf_jailtraffic_ssi.md) :: fwcounters

<!---
## Useful stuff,errata,tips,hints etc

- [Convert jails from EZJail to CBSD](/ezjail2cbsd_ssi.md) :: (hint by: Nikita Druba **LordNicky**)

## Helpers/Modules

- [Working with packages and pkg(7) in jail via CBSD](/modules/pkg.d_ssi.md) :: pkg.d
- [Working with passwd(1), sysrc(8), service(8) in jail via CBSD](/modules/bsdconf.d_ssi.md) :: bsdconf.d
- [Synchronization of jail environments via csync2 and CBSD csync2 module](/modules/csync2.d_ssi.md) :: bsdconf.d

## CBSD Jail: mass management

- [Manage CBSD with Puppet](/wf_puppet_ssi.md) :: CBSD and Puppet
- [Working with CBSD through Shell scripts](/wf_script_mass_man_ssi.md) :: CBSD и Shell Scripts

--->

## VM: Operation with virtual machine via bhyve

[Single file](/workflow_bhyvecbsd.md)

- [VM create](/wf_bcreate_ssi.md) :: bcreate, bconstruct-tui
- [bhyve cloud-init with CBSD](/wf_bhyve_cloudinit_ssi.md) :: bconstruct-tui, cloudinit
- [bhyve create via CBSDfile](/cbsdfile.md) :: up, destroy
- [VM config](/wf_bconfig_ssi.md) :: bconfig
- [Custom behavior settings by exit codes](/wf_bexit_behavior_ssi.md) :: bconfig, bset, bconstruct-tui
- [bhyve virtual disk](/wf_bstorage_ssi.md) :: bconfig, bset, bhyve-dsk
- [Bhyve CPU Topology](/wf_bcpu_topology_ssi.md) :: cpu-topology, vm-cpu-topology, vm-cpu-topology-tui
- [starting and stoping VM](/wf_bstop_bstart_ssi.md) :: bstart, bstop
- [bhyve network options](/wf_bhyvenetwork_ssi.md) :: bcreate, bconfig
- [bhyve PCI Passthrough and SR-IOV](/wf_bhyveppt_ssi.md) :: bhyve-ppt
- [Shared folders for bhyve vm](/wf_bhyve_p9_ssi.md) :: bhyve-p9shares
- [VM starting order](/wf_jorder_ssi.md) :: border
- [VM removal](/wf_bremove_ssi.md) :: bremove
- [VM renaming](/wf_brename_ssi.md) :: brename
- [VMs list](/wf_bls_ssi.md) :: bls
- [Using VNC with bhyve](/wf_bvnc_ssi.md) :: bconfig
- [Attach to console](/wf_blogin_ssi.md) :: blogin
- [VM cloning](/wf_bclone_ssi.md) :: b\[r\]clone
- [VM export](/wf_bexport_ssi.md) :: bexport
- [VM import](/wf_bimport_ssi.md) :: bimport
- [VM bhyve checkpoints, suspend and pauses](/wf_bcheckpoint_ssi.md) :: bsuspend, bcheckpoint, bpause
- [VM bhyve live migration](/wf_bmigration_ssi.md) :: bmigrate
- [Running bhyve via debugger](/wf_bhyve_gdb_ssi.md) :: debug engine

<!---
## Operation with nodes

[Single file](/node_cbsd.md)

- [What nodes is meant](/wf_node_overview_ssi.md) :: node
- [list of nodes](/wf_node_list_ssi.md) :: node
- [adding nodes](/wf_node_add_ssi.md) :: node
- [removal nodes](/wf_node_del_ssi.md) :: node
- [execute commands on remote nodes](/wf_node_rexe_ssi.md) :: rexe
- [Login into node by CBSD user via ssh](/wf_nlogin_ssi.md) :: nlogin
--->
