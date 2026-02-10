# Operaion with repository

## repo command

```
			% cbsd repo

```

**Description**:

Work with a repository of bases, kernels and images. The quantity a repository can be more than one and they are specified through a gap in the file repo variable _$workdir/nc.inventory_. Downloading will occur from the first repository where the object will find. Respectively, if the local repository is used — it should be the first.

obligatory parameters:

- **action** — can accept value _list_ (get list), _get_ (download), _put_ (upload)

Arguments which in certain cases aren't obligatory:

- **sources** — sources for **action** — with what data we want to work. Can accept values:

  - _src_ — source code for OS (${workdir}/src)
  - _obj_ — object file for source code ($workdir}/obj)
  - _base_ — world/bases ($workdir/base)
  - _kernel_ — kernels of OS (${workdir}/base)
  - _img_ — jails
- **name** — name (used with sources=obj,base,kernel,img — name of base/world or jails)
- **stable** — related to sources=obj,base — get RELENG\_X instead of RELENG\_X\_Y
- **ver** — By default, for obtaining the list or downloading the current version of OS will be used. With the ver=X.Y parameter it is possible to specify other version for jail/base/kernel. At ver=any for action=list, will be it is deduced all available data of sources for all versions

**Example**:

Obtaining the list of available jails for 9.1 versions

```
			% cbsd repo action=list sources=img ver=9.1

```

to get and import kfreebsd jail:

```
			% cbsd repo action=get sources=img name=kfreebsd

```

![](http://www.convectix.com/img/repo1.png)

Upon termination of an import the question of correct IP for a new jails will be asked and whether to create alias automatically. We choose COMMIT for preservation.

![](http://www.convectix.com/img/repo2.png)

Now jail in system also it is possible to use

![](http://www.convectix.com/img/repo3.png)

Copyright © 2013—2024 CBSD Team.

