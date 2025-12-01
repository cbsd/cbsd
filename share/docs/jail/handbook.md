# jail container create via dialog menu

## Commands: jcreate, jconstruct-tui, up

**Description:**

The easiest way to create a virtual machine is with the TUI interface (`cbsd jconstruct-tui`).
However, if you frequently create environments, it is recommended to use a CBSDfile method.
Also, if you like typing commands in the console, you might like `cbsd jcreate` with args (or config);

Example 1, TUI:

```
% cbsd jconstruct-tui
```

The minimum number of steps you need to take to create a jail is give the environmnet a short, unique name ( `jname ` ).


![freebsd-jcreate](https://convectix.com/img/jcreate1.png?raw=true)

Example 2, CLI or config:

```
% cbsd jcreate --help
% cbsd jcreate jname=test1 runasap=4
% cbsd bcreate jconf=/path/to/config.jconf
```

Example 3, CBSDfile:
```
jail_test2()
{
	vnet=1

	ip4_addr="10.0.0.2/24"		# vnet/guest IPv4
	ci_gw4="10.0.0.1"		# gw

	pkglist="misc/mc shells/bash"	# additional packages in jail

	astart=1			# auto-start?

	runasap=1
}
```
```
cbsd up
```

Using the CBSDfile, you can override parameters via the command line, e.g.:
```
cbsd up ver=14.3 astart=0 vnet=0 ip4_addr=10.0.0.5,::2"
```

## jail list

Use the `cbsd jls` command to get a list of jail containers:
```
cbsd jls
```

You can adjust the output of fields and use filters:
```
cbsd jls display=jname,ip4_addr,ver header=0
```

```
cbsd jls astart=0			# ( display jails which have the 'astart' flag set to '0' value )
cbsd jls ip4_addr=10.0.0.1		# ( display jails which have 'ip4_addr' == '10.0.0.1' )
```

## jail login, jail exec

To log into the container console as the root (by default) user, use the command: `cbsd jlogin`

To execute command without login, please use the command: `cbsd jexec`. Multiple jails (via mask) supported:

![cbsd-jexec-login](https://convectix.com/img/cbsd_jexec_login1.png?raw=true)


## other common commands and operations

stop jail: `cbsd jstop --help`

start jail: `cbsd jstart --help`

reconfigure jail: `cbsd jconfig --help`

destroy jail: `cbsd jdestroy --help`  (or `cbsd destroy` for CBSDfile)

get jail params: `cbsd jget --help`

set jail params: `cbsd jset --help`

Full `jail`-module commands: `cbsd help module=jail`

