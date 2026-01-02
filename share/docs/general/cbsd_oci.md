# CBSD and OCI containers

## Description

The CBSD can use two formats for distributing virtual machine and container images - its own and [OCI containers](https://opencontainers.org/).
There is a misconception that with the advent of OCI all container managers on FreeBSD are deprecated.

There are a few points to note here:

- OCI is an image standard, it does not regulate how exactly to work with the image;
- At the time of writing this article (2025), OCI takes into account and focuses on the capabilities of Linux systems and in particular, the use of cgroups/FS layers/namespaces. Thus, many things (capabilities, plugins) on FreeBSD marked as: "not supported yet". 
As an example - namespaces. For example, in OCI, containers may have 'sysctl' parameters which is widely used (e.g. in redis or postgresql containers). 
However, FreeBSD won't let you use this because it doesn't have such capabilities.
```
# A list of sysctls to be set in containers by default,
# specified as "name=value",
# for example:"net.ipv4.ping_group_range=0 0".
#
default_sysctls = [
  "net.ipv4.ping_group_range=0 0",
]
```
- The official **FreeBSD Handbook** [describes](https://docs.freebsd.org/en/books/handbook/jails/) classic jail as `FreeBSD base` mounted as RO (nullfs) and overlay data mounted in RW (nullfs). 
  This is a fundamental difference in the approach to using images, and when someone says "FreeBSD jail managers is obsolete", this approach is meant. 
  However, it is a relevant approach for FreeBSD. Moreover, nothing prevents you from using it in OCI images oriented for FreeBSD, at least CBSD allows it (and you save about ~500 megabytes in the case of a full FreeBSD base distribution and gives you a full set of debugging utilities "for free" inside jails).

## How to work with OCI

The integration of CBSD and the OCI is achieved using a `buildah` tools. 
For this reason, if you plan to use OCI images - you must install package and run CBSD reinitialization:
```
pkg install -y buildah
cbsd initenv
```

If the `buildah` utility is installed, CBSD will start using OCI images automatically in addition to its own images. Check out the current examples in:
```
cbsd images --help
cbsd jstart --help
```

Example1: native FreeBSD image (base-in-pkg-based):

```
cbsd jcreate jname=test ver=empty baserw=1 pkg_bootstrap=0 floatresolv=0 applytpl=0 etcupdate_init=0 from=docker.io/convectix/freebsd14-base runasap=1
cbsd jlogin test

root@test:~ # telnet
Command 'telnet' not found, but can be installed with:
pkg install -y FreeBSD-telnet
```

Example1: Linux image (needs 'kldload linux.ko' and 'kldload linux64.ko`)

```
cbsd jcreate jname=test2 ver=empty baserw=1 pkg_bootstrap=0 floatresolv=0 applytpl=0 etcupdate_init=0 exec_start=/bin/true exec_stop=/bin/true from=docker.io/library/alpine emulator=linux runasap=1
cbsd jlogin test

cbsd@test2> uname -a
Linux test2.my.domain 5.15.0 FreeBSD 14.2-RELEASE releng/14.2-n269506-c8918d6c741 GENERIC x86_64 Linux
cbsd@test2> cat /etc/os-release 
NAME="Alpine Linux"
ID=alpine
VERSION_ID=3.20.3
PRETTY_NAME="Alpine Linux v3.20"
HOME_URL="https://alpinelinux.org/"
BUG_REPORT_URL="https://gitlab.alpinelinux.org/alpine/aports/-/issues"
```

## Errata

- Support for `buildah/OCI` is experimental (Also `buildah` package is marked as experimental by itself) - do not use it in production;
- CBSD uses `buildah` tool only to get an image (or generate and push a jail container to the Docker registry);
- CBSD uses a `buildah` with alternative paths (to store data in the CBSD hier/structure). If you have difficulties with the `buildah` images via CBSD, call it with the appropriate arguments (check out `cbsd buildah` output).
- CBSD does not act as a supervisor for the process specified as an entrypoint ( or any other health check functions )
- On ZFS and non-ZFS system, images are stored in different places. With ZFS, the CBSD uses a snapshot and works as a `zfs_snapsrc` parameter when creating a container. If the ZFS is absent, 
  the image is located in the ~cbsd/basejail/ directory and the container is created by copying files (which, of course, is much slower compared to CoW).
  Images on a non-ZFS system are also visible as 'cbsd bases: 
- The OCI hier currently doesn't support the FreeBSD approach: "mount the RO base" and RW data in an overlay;
- At the moment, there is no known vendor that makes NATIVE images for FreeBSD for their services.


All containers you can try now (~2026y) will require Linuxulator, which has very limited capabilities. 
In other words, no vendor will support software if they find it running on FreeBSD via Linuxulator. 
Therefore, if you're using a Linux container, you're unlikely to get support, and be prepared to be left alone with the problem.


Perhaps the situation with native OCI images for FreeBSD will change in the future — for example, the idea of integrating OCI image generation 
into the FreeBSD ports framework is a good solution. Imagine if some ports with services also had a container file (e.g., `/usr/ports/www/nginx/Containerfile` ) 
that allowed you to run the `make ociimage` command!


