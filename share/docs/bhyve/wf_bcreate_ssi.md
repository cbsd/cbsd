# bhyve virtual machine create via dialog menu

## Commands: bcreate, bconstruct-tui

```
% cbsd bconstruct-tui
% cbsd bcreate jconf=/path/to/conf.jconf
```

**Description:**

The easiest way to create a virtual machine is with the TUI interface (`cbsd bconstruct-tui`). 
However, if you frequently create environments, it is recommended to use a CBSDFile method.
Also, if you like typing commands in the console, you might like `cbsd bcreate` with args.

Examples

```
% cbsd bconstruct-tui
```

```
% cbsd bcreate --help
% cbsd bcreate jname=vm1 vm_os_type=linux vm_os_profile=Debian-13-x86_64 vm_ram=2g vm_cpus=1 runasap=1 imgsize=10g
```

or CBSDFile:
```
bhyve_vm1()
{
	vm_ram="2g"
	vm_cpus="1"

	imgsize="105g"

	vm_os_type="linux"
	vm_os_profile="Debian-13-x86_64"

	runasap=1
	ssh_wait=0
}
```

## Virtual Machine Profiles

Profiles that come with **CBSD** and on which systems determine the URL from which the ISO image is downloaded,
placed in the catalog _~workdir/etc/defaults/_ and start with the prefix **vm-**

. For example, you can see the profiles list by command:

```
% ls -1 ~cbsd/etc/defaults | grep ^vm-
vm-dflybsd-x86-5.conf
vm-freebsd-FreeBSD-x64-11.1.conf
vm-freebsd-FreeBSD-x64-12.0-LATEST.conf
vm-freebsd-FreeNAS-x64-11.conf
vm-freebsd-pfSense-2-RELEASE-amd64.conf
vm-linux-CentOS-7.4-x86_64.conf
vm-linux-Debian-x86-9.conf
vm-linux-fedora-server-26-x86_64.conf

```

Since the **CBSD** releases come out much less often than the versions of the various distributions, these profiles may expire quickly enough, and as a result, the image becomes inaccessible by the old links.

In order not to get into similar situations and get new profiles, you can update them separately from **CBSD** with the Makefile in the _~workdir/etc_ directory. There are two commands in the Makefile (you will see them if you just write make in this directory):

- make profiles-create - is done once by starting the git repository from the GitHub: [https://github.com/cbsd/cbsd-vmprofiles](https://github.com/cbsd/cbsd-vmprofiles)
- make profiles-update - is done every time (after profiles-create) when you want to update profiles

Since these operations are used by git, you must first install it in the system: pkg install devel/git (or from the ports: make -C /usr/ports/devel/git install)

For example:

```
% cd ~cbsd/etc
% make profiles-create
% make profiles-update

```

If you notice that some version of the profile is out of date and in the repository [https://github.com/cbsd/cbsd-vmprofiles](https://github.com/cbsd/cbsd-vmprofiles) no one sent a correction, you can contribute **CBSD** by sending changes (in the old profile or creating a new one)
independently through Pull Request, having an account on github.com

## Creating your own virtual machine profiles

Since virtual machines in the CBSD are created using profiles, you may want to create your own profile. 
You can use any existing profiles as examples, or use a helper script (also available from `cbsd bconstruct-tui`):
```
% cbsd vm-profile-wizard
```

It works in question-answer mode and generates a skeleton (it may be sufficient) in the user's custom config directory: ~cbsd/etc/

If you've created a profile of a public OS, you can submit it to the CBSD and make other users happy: [https://github.com/cbsd/cbsd-vmprofiles](https://github.com/cbsd/cbsd-vmprofiles)
