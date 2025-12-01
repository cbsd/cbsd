# **CBSD** FreeBSD JAIL quick start

The [jail(8)](https://man.freebsd.org/jail/8) mechanism is an implementation of [FreeBSD's](https://www.freebsd.org/) OS-level virtualisation 
that allows system administrators to partition a FreeBSD-derived computer system into several independent mini-systems called jails, all sharing the same kernel, with very little overhead.

## Method 1: TUI/DIALOG

If you are a beginner and/or create containers in small quantities, then TUI is the easiest option, since it does not require you to remember syntax/arguments or any parameters - you will see everything yourself:

```
cbsd jconstruct-tui
```

Enter the container name (`jname` parameter) and you can press 'GO' button to create your first container.

![freebsd-jcreate](https://convectix.com/img/jcreate1.png?raw=true)

## Method 2: CLI/arguments

All the jail parameters that you saw in TUI, you can use as arguments for `jcreate`, e.g.:
```
cbsd jcreate jname=myjail runasap=1
```

_hint: see other examples: `cbsd jcreate --help`_

## Method 3: CBSDfile

Finally, the third option (which is convenient to use with `git` if you often have to create and destroy containers on multiple hosts) is to use [CBSDfile](../general/cbsdfile.md) file, e.g., create */tmp/CBSDfile* file with the following contents:
```
jail_myjail1()
{
    ver="native"
    host_hostname="${jname}.my.domain"
    astart=1
    pkg_bootstrap=0
}
```

Then use `cbsd up` in the directory where CBSDfile is located:
```
cd /tmp
cbsd up
```

See also: [CBSD + jail handbook](jail/handbook.md)
