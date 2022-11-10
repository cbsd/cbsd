# CBSD Project

Copyright (c) 2013-2022, The CBSD Development Team

Homepage: https://bsdstore.ru

## Description

Yet one more wrapper around [jail](https://man.freebsd.org/jail/8), 
[Xen](http://www.xenproject.org/) and [bhyve](https://man.freebsd.org/bhyve/4) for [FreeBSD](https://www.freebsd.org).

![demo](https://www.bsdstore.ru/gif/jdemo.gif)
![demo](https://www.bsdstore.ru/gif/bdemo.gif)

## Usage

Quick start: https://www.bsdstore.ru/en/cbsd_quickstart.html

For installation and usage see: https://www.bsdstore.ru/en/docs.html

## Clastering opportunities

* Pool-bindend clustering demo:

- part1, for jails: https://youtu.be/f3IrLjbr3b4
- part2, for bhyve (+live migration): https://youtu.be/kC_xyTbRyl4

* Multi-DC VPC/vxlan-based articles:

- https://www.bsdstore.ru/en/13.0.x/wf_vpc_ssi.html

## Limitations

Tested with following OSes and distribution:

- FreeBSD 13.0+
- HardenedBSD

## Support

* For CBSD-related support, discussion and talks, please join to Telegram CBSD usergroup channel: @cbsdofficial
* Web link: https://t.me/cbsdofficial
* Or subscribe to mailing list by sending email to: cbsd+subscribe@lists.tilda.center
* Other contact: https://www.bsdstore.ru/en/feedback.html

## Stargazers over time

[![Stargazers over time](https://starchart.cc/cbsd/cbsd.svg)](https://starchart.cc/cbsd/cbsd)

## Contributing

* Fork me on GitHub: [https://github.com/cbsd/cbsd.git](https://github.com/cbsd/cbsd.git)
* Switch to 'develop' branch
* Commit your changes (`git commit -am 'Added some feature'`)
* Push to the branch (`git push`)
* Create new Pull Request

### Installing development version

a) First install the required dependencies:

```
pkg install sudo libssh2 rsync sqlite3 git pkgconf
```

b) get the latest version of **CBSD**:

```
git clone https://github.com/cbsd/cbsd.git /usr/local/cbsd
```

c) create a **CBSD** user:

```
pw useradd cbsd -s /bin/sh -d /nonexistent -c "cbsd user"
```

d) create links of the rc.d scripts to start **CBSD** at system startup and create link to bsdconfig module:

```
cd /usr/local/etc/rc.d
ln -s /usr/local/cbsd/rc.d/cbsdd
ln -s /usr/local/cbsd/rc.d/cbsdrsyncd
mkdir -p /usr/local/libexec/bsdconfig
ln -s /usr/local/cbsd/share/bsdconfig/cbsd /usr/local/libexec/bsdconfig/cbsd
```

e) Follow 'initial setup' section [as usual](https://www.bsdstore.ru/en/installing_cbsd.html):

```
env workdir=/usr/jails /usr/local/cbsd/sudoexec/initenv
```

Notes: when you run the `initenv` script, all binaries and components will be compiled and installed.

## Contributors

### Code Contributors

This project exists thanks to all the people who contribute. [See the contributors list](https://github.com/cbsd/cbsd/graphs/contributors).

### Financial Contributors

Become a financial contributor and help us sustain our community.

<a href="https://www.patreon.com/clonos"><img src="https://c5.patreon.com/external/logo/become_a_patron_button@2x.png" alt="Patreon donate button" /></a>
