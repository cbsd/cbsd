# bhyve virtual machine create via dialog menu

## Commands: bcreate, bconstruct-tui, up

**Description:**

The easiest way to create a virtual machine is with the TUI interface (`cbsd bconstruct-tui`).
However, if you frequently create environments, it is recommended to use a CBSDFile method.
Also, if you like typing commands in the console, you might like `cbsd bcreate` with args (or config);

Example 1, TUI:

```
% cbsd bconstruct-tui
```

The minimum number of steps you need to take to create a virtual machine is to select a profile and give the virtual machine a short, unique name ( `jname ` ).


![](http://www.convectix.com/img/cbsd_bconstruct-tui1.png)

hint: The lightning bolt icon next to the profiles indicates that this image has already been received by the system.

Example 2, CLI or config:

```
% cbsd bcreate --help
% cbsd bcreate jname=vm1 vm_os_type=linux vm_os_profile=Debian-13-x86_64 vm_ram=2g vm_cpus=1 runasap=1 imgsize=10g
% cbsd bcreate jconf=/path/to/config.jconf
```

Example 3, CBSDFile:
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
![](http://www.convectix.com/img/cbsd_w1.png)

- Enter URL or full path to ISO, e.g.: /windows11.iso
![](http://www.convectix.com/img/cbsd_w2.png)

- After a series of intuitive questions, we'll be taken to the final screen, where you can once again adjust the starting parameters 
(don't worry if you made a mistake - you can use a text editor to correct and adjust any parameters that weren't in the wizard):
![](http://www.convectix.com/img/cbsd_w3.png)

If you use a `virtio-blk` disk type, do not forget to specify the path in the Windows installer to the drivers that you want to have 
on the second virtual CD-ROM:
![](http://www.convectix.com/img/cbsd_w4.png)

- we use win11 bhyve guest:
![](http://www.convectix.com/img/cbsd_w5.png)

- Most likely you will have to do the same for the (VIRTIO-based) network card after installing the system:
![](http://www.convectix.com/img/cbsd_w6.png)

- If the driver is correct, you will see the disk. If the disk is not visible, try `ahci-hd` disk type or `NVMe`:
![](http://www.convectix.com/img/cbsd_w7.png)


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

![](http://www.convectix.com/img/cbsd_tmp1.png)


## bhyve boot order device

The CBSD supports two options for managing boot devices for bhyve: by installing a boot device on the first PCI BUS or using UEFI VARS boot order.
Please note that UEFI VARS take precedence over the first method. Please use the built-in help and examples via the appropriate script:
```
cbsd bhyve-efivar --help
```

Or navigate via TUI: `cbsd bconfig` -> `vm_boot`.

![](http://www.convectix.com/img/cbsd_border1.png)

![](http://www.convectix.com/img/cbsd_border2.png)

:bangbang: | :warning: Please note! VARS are dynamic. If you add a new device (for example, a CD-ROM drive), you must start the virtual machine once for the CBSD to see the entries.
:---: | :---


## CBSD bhyve CLOUD IMAGES

In addition to classic images using original ISO images, CBSD supplies profiles for cloud images and acts as `NoCloud` cloud-init Datasource!
By using these features, you can save significant time and get a working system in just a few seconds, bypassing the long and routine installation phase.

In this case, CBSD download only a small minimal image of an already working system, and all subsequent virtual environments of this profile will use it as a golden image 
(in the case of a ZFS file system, you will receive a CoW for guests).

## CBSDFile method

You can create CBSDFile directories of your virtual appliance (and distribute them between hosts, for example, via Git). Example of CBSDFile for a Ubuntu 24 server cloud image:
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
You can omit some parameters from the CBSDFile and move them to the global configuration file, for example: ~cbsd/etc/bhyve-default-default.conf
```
ci_gw4="10.0.0.1"
interface="bridge1"
```

Also if you use 'DHCP' in ip4_addr ( ci_ip4_addr ), make sure the `nodeippool` in `cbsd initenv-tui` is the pool you expect.

## other common commands and operations

stop VM: `cbsd bstop --help`
start VM: `cbsd bstart --help`
reconfigure VM: `cbsd bconfig --help`
destroy VM: `cbsd bdestroy --help`  (or `cbsd destroy` for CBSDFile)
register new ISO image: `cbsd media --help`
manage virtual disks: `cbsd bhyve-dsk --help`
configure PCI Passtru devices: `cbsd bhyve-ppt --help`
configure P9FS (9P) shares: `bhyve-p9shares --help`
manage bhyve checkpoints: `cbsd bcheckpoint --help`

Full `bhyve`-module commands: `cbsd help module=bhyve`

