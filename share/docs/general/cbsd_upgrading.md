# **CBSD** Upgrading

When you get a new version of CBSD (see current version via: `cbsd version`), your working directories continue to work with the data that was initialized by the previous version.
Various upgrades may require running data migration scripts (for example, changing the SQLite3 table structure). This should be done manually so that you are prepared for "possible problems":

```
cbsd initenv
```

Please note that the CBSD upgrade procedure does not require a forced restart of virtual environments or the `cbsd` service - this operation should not disrupt the functionality of your containers or virtual machines.
As for "possible problems" during the upgrade - we hope that you will not encounter them. However, cbsd provides some features designed to reduce risks:

1)

The CBSD has directories for 'pre'/'post' hooks, in which you can place arbitrary scripts that work before and after the init. 
So, these scripts can send a notification and perform a backup (or import, export or migration) of virtual environments.

To do this, create in the workdir a directory named `upgrade`:
```
mkdir -p ~cbsd/upgrade
```

Any scripts that start with *pre-initenv-* or *post-initenv-* and have an executable flag will be executed before modifying initenv or after, respectively.

2)

You can see an example of such a script in the default CBSD ( _/usr/local/cbsd/upgrade/backup_db/pre-initenv-backup_ ), which works by default and creates a backup copy of the main database ( ~cbsd/var/db directory )

![cbsd cmd help](https://convectix.com/img/cbsd-upgrading1.png)
