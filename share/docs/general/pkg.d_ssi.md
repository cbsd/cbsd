# Working with packages and pkg(7) in jails via CBSD

## Command: pkg

```sh
cbsd pkg
```

**Description**:

**cbsd pkg** \- is wrapper around standard FreeBSD [pkg(7)](http://man.freebsd.org/pkg/7) tools to use **jname** argument for more comfort work with the jail from the master host

Via **mode=** argument indicating a needet action. Values can be:

- **add, install** \- to install packages
- **remove** \- to remove packages
- **bootstrap** \- init pkg (normally done in the jail one times on creating)
- **info, query** \- execute queries _info_ or _query_ with the same syntax pkg
- **update** \- execute pkg update
- **upgrade** \- execute upgrade
- **clean** \- execute clean to purge pkg cache

For some commands (clean, update, upgrade) it is permissible jname= to specify as mask for performing the operation simultaneously in several jails

Keep in mind that must first be specified parameters **mode** and **jname**. All that comes after - not analyzed and treated [pkg(7)](http://man.freebsd.org/pkg/7) as is.

In addition, please note that all operations are performed with the set environment variables **ASSUME\_ALWAYS\_YES=yes** and **IGNORE\_OSVERSION=yes** to suppress the interactivity that basically, you need to work in automated scripts. If for some reason this does not work for you, use [cbsd rexe](http://www.convectix.com/en/13.0.x/wf_jexec_ssi.html) to work with pkg directly.

**Example1:** Update pkg index files inside ALL containers:

```sh
cbsd pkg mode=update jname='*'
```

**Example2:** Update ALL packages inside containers, whose name starts with redis\*:

```sh
cbsd pkg mode=upgrade jname='redis*'
```

**Example3:** Clear pkg cache in ALL containers:

```sh
cbsd pkg mode=clean jname='*'
```

**Example4:** Get installed packages for box1 and for all jails with jname mask 'jail\*' (in **CBSD 11.2.1+**):

```sh
cbsd pkg mode=query jname=box1 %o
cbsd pkg mode=query jname='jail*' %o
```

**Example5:** Install **bash, mc, wget** in mytest1 jail and **nginx-devel,mysql57-server,postgresql96-server,mc** for all jails with jname mask 'jail\*' (in **CBSD 11.2.1+**:

```sh
cbsd pkg mode=install jname=mytest1 bash mc wget
cbsd pkg jname='jail*' mode=install nginx-devel mysql57-server postgresql96-server mc

```

or that much better (in order to avoid the same name in different categories) indicate origin package, not the name:

```sh
cbsd pkg mode=install jname=mytest1 shells/bash ftp/wget misc/mc
```

**Example6:** Upgrade mc package in jail1:

```sh
cbsd pkg mode=upgrade jname=jail1 mc
```

**Example7:** Remove wget and lsof packages in box1 and mc from all jails with jname mask 'jail\*' (in **CBSD 11.2.1+**:

```sh
cbsd pkg mode=remove jname=box1 wget lsof
cbsd pkg jname='jail*' mode=remove mc

```

Copyright © 2013—2024 CBSD Team.

