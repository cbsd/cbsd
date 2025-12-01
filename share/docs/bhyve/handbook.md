# bhyve virtual machine create via dialog menu

## Commands: bcreate, bconstruct-tui, up

**Description:**

The easiest way to create a virtual machine is with the TUI interface (`cbsd bconstruct-tui`).
However, if you frequently create environments, it is recommended to use a CBSDfile method.
Also, if you like typing commands in the console, you might like `cbsd bcreate` with args (or config);

Example 1, TUI:

```
% cbsd bconstruct-tui
```

![bconstruct-tui1](https://convectix.com/img/bconstruct-tui1.png?raw=true)

The minimum number of steps you need to take to create a virtual machine is to select a profile and give the virtual machine a short, unique name ( `jname ` ).

![bconstruct-tui2](https://convectix.com/img/bconstruct-tui2.png?raw=true)

hint: The lightning bolt icon next to the profiles indicates that this image has already been received by the system.

Example 2, CLI or config:

```
% cbsd bcreate --help
% cbsd bcreate jname=vm1 vm_os_type=linux vm_os_profile=Debian-13-x86_64 vm_ram=2g vm_cpus=1 runasap=1 imgsize=10g
% cbsd bcreate jconf=/path/to/config.jconf
```

Example 3, CBSDfile:
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
```
cbsd up
```

## Virtual Machine Profiles

Profiles are grouped names of parameters that describe the virtual machine being created. 
They can contain guest flavor or URL to the image source.

By default, the CBSD comes with a number of pre-built (contrib) profiles for an easy start.
Please refer to the built-in documentation for a list of profiles:
```
cbsd get-profiles --help
```

Contrib profiles placed in the catalog _~workdir/etc/defaults/_ and start with the prefix **vm-**(*.conf).
If you want to modify (change or create a new profile), please use the directory one level higher - do not edit system profiles, 
because all changes will be deleted after `cbsd initenv`: _~workdir/etc/_

If you notice that some version of the profile is out of date and in the repository [https://github.com/cbsd/cbsd-vmprofiles](https://github.com/cbsd/cbsd-vmprofiles) no one sent a correction, 
you can contribute **CBSD** by sending changes (in the old profile or creating a new one) independently through Pull Request, having an account on github.com

## Creating your own virtual machine profiles with helper

You can use any existing profiles as examples and create your profile in any text editor.
There is also a helper script that will make creating your profile easier:
```
% cbsd vm-profile-wizard
```

Or use 'Profile wizard>' buttin in `cbsd bconstruct-tui`.
It works in question-answer mode and generates a skeleton (it may be sufficient) in the user's custom config directory: ~cbsd/etc/
If you've created a profile of a public OS, you can submit it to the CBSD and make other users happy: [https://github.com/cbsd/cbsd-vmprofiles](https://github.com/cbsd/cbsd-vmprofiles)

Below is an example of creating a Windows11 bhyve profile, assuming you downloaded an ISO image. In this example, we use `/windows11.iso`.

- Run wizard from `cbsd bconstruct-tui`:

![bconstruct-wizard1](https://convectix.com/img/bconstruct-wizard1.png?raw=true)

- Enter URL or full path to ISO, e.g.: /windows11.iso

![bconstruct-wizard2](https://convectix.com/img/bconstruct-wizard2.png?raw=true)

- After a series of intuitive questions, we'll be taken to the final screen, where you can once again adjust the starting parameters 
(don't worry if you made a mistake - you can use a text editor to correct and adjust any parameters that weren't in the wizard):

![bconstruct-wizard3](https://convectix.com/img/bconstruct-wizard3.png?raw=true)

If you use a `virtio-blk` disk type, do not forget to specify the path in the Windows installer to the drivers that you want to have 
on the second virtual CD-ROM:

![bconstruct-wizard4](https://convectix.com/img/bconstruct-wizard4.png?raw=true)

- we use win11 bhyve guest:

![bconstruct-wizard5](https://convectix.com/img/bconstruct-wizard5.png?raw=true)

- Most likely you will have to do the same for the (VIRTIO-based) network card after installing the system:

![bconstruct-wizard6](https://convectix.com/img/bconstruct-wizard6.png?raw=true)

- If the driver is correct, you will see the disk. If the disk is not visible, try `ahci-hd` disk type or `NVMe`:

## bhyve TPM

Some guest systems (for example Windows 11) require the presence of TPM2.0 devices. 
You can PASSTHRU the host TPM ( /dev/tpm ) `kldload tpm`-required. Or emulate TPM via `swtpm` package. Please install it first and re-run CBSD init:
```
pkg install -y swtpm
cbsd initenv
```

Please use the built-in help and examples via the appropriate script:
```
cbsd blpc --help
```

Or navigate via TUI: `cbsd bconfig` -> `LPC` -> `tpm` -> set the parameter to `new` value:

![bconstruct-tpm2.png](https://convectix.com/img/bconstruct-tpm2.png?raw=true)

## bhyve boot order device

The CBSD supports two options for managing boot devices for bhyve: by installing a boot device on the first PCI BUS or using UEFI VARS boot order.
Please note that UEFI VARS take precedence over the first method. Please use the built-in help and examples via the appropriate script:
```
cbsd bhyve-efivar --help
```

Or navigate via TUI: `cbsd bconfig` -> `vm_boot`.

![cbsd_border1.png](https://convectix.com/img/cbsd_border1.png?raw=true)

![cbsd_border2.png](https://convectix.com/img/cbsd_border2.png?raw=true)

:bangbang: | :warning: Please note! VARS are dynamic. If you add a new device (for example, a CD-ROM drive), you must start the virtual machine once for the CBSD to see the entries.
:---: | :---


## CBSD bhyve CLOUD IMAGES

In addition to classic images using original ISO images, CBSD supplies profiles for cloud images and acts as `NoCloud` cloud-init Datasource!
By using these features, you can save significant time and get a working system in just a few seconds, bypassing the long and routine installation phase.

In this case, CBSD download only a small minimal image of an already working system, and all subsequent virtual environments of this profile will use it as a golden image 
(in the case of a ZFS file system, you will receive a CoW for guests).

## CBSDfile method

You can create CBSDfile directories of your virtual appliance (and distribute them between hosts, for example, via Git). Example of CBSDfile for a Ubuntu 24 server cloud image:
```
bhyve_ub1()
{
	vm_ram="4g"
	vm_cpus="2"

	imgsize="5g"    # cannot be smaller than the original image

	vm_os_type="linux"
	# based on ubuntuserver 22.04 cloud:
	vm_os_profile="cloud-ubuntuserver-amd64-22.04"

	ip4_addr="DHCP"
	# bhyve-default-default.conf :
	# ci_gw4="10.0.0.1"

	ci_jname="${jname}"
	ci_fqdn="${fqdn}"
	ci_ip4_addr="${ip4_addr}"
	ci_gw4="${ip4_gw}"

	runasap=1
	ssh_wait=1
}

# post-start custom
postcreate_ubcloud1()
{
	## copy customization script from current directory and exec:
	# bscp prepare.sh ${jname}:prepare.sh
	# bexec sudo ./prepare.sh

	# or for simple action: install 'fish' package
	bexec << EOF
sudo apt install -y fish
EOF
}
```

Just run:
```
cbsd up
```

and in a few seconds you can log into a working virtual machine with Ubuntu 24:
```
cbsd blogin
```

Notes: Cloud images are contextualized when created, so unlike ISO images, you must specify the correct network settings in advance.
You can omit some parameters from the CBSDfile and move them to the global configuration file, for example: ~cbsd/etc/bhyve-default-default.conf
```
ci_gw4="10.0.0.1"
interface="bridge1"
```

Also if you use 'DHCP' in ip4_addr ( ci_ip4_addr ), make sure the `nodeippool` in `cbsd initenv-tui` is the pool you expect.

## Bhyve login, VNC console

If you're using classic ISO images, use the VNC client (e.g. tigervnc-viewer) to access the graphical console.

If you're using CLOUD images, you'll get a working environment immediately and can log into the virtual machine as a cloud user using the `cbsd blogin` command:

![cbsd-blogin](https://convectix.com/img/cbsd_blogin1.png?raw=true)


## Bhyve PCI passthru

CBSD allows you to configure bhyve PCI passthru more easily. To do this, carefully study the built-in documentation:
```
cbsd bnyve-ppt --help
```

<img src="https://convectix.com/img/bhyve-ppt1.png" width="1024" title="bhyve-ppt list" alt="bhyve-ppt list"/>

## other common commands and operations

stop VM: `cbsd bstop --help`

start VM: `cbsd bstart --help`

reconfigure VM: `cbsd bconfig --help`

destroy VM: `cbsd bdestroy --help`  (or `cbsd destroy` for CBSDfile)

manage (attach/detach/remove new ISO image): `cbsd media --help`

manage virtual disks: `cbsd bhyve-dsk --help`

configure P9FS (9P) shares: `bhyve-p9shares --help`

manage bhyve checkpoints: `cbsd bcheckpoint --help`

edit bhyve CPU topology: `vm-cpu-topology-tui --help`

edit bhyve Flavor/packages: `cbsd vm-packages-tui --help`

Full `bhyve`-module commands: `cbsd help module=bhyve`

