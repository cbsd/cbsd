#!/bin/sh

[ -f /usr/jails/share/FreeBSD-jail-kfreebsd-wheezy-skel/bin/bash ] && exit 0
debootstrap wheezy /usr/jails/share/FreeBSD-jail-kfreebsd-wheezy-skel http://ftp.us.debian.org/debian

