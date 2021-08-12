### To get CBSD work in DFLYBSD platform, you need to install extra packages:

% pkg ins sysrc libelf pkgconf sqlite3 rsync


Also please get next files from the bsdconfig distribution:

/usr/libexec/bsdconfig/include/messages.subr
/usr/share/bsdconfig/common.subr
/usr/share/bsdconfig/dialog.subr
/usr/share/bsdconfig/mustberoot.subr
/usr/share/bsdconfig/strings.subr
/usr/share/bsdconfig/struct.subr
/usr/share/bsdconfig/variable.subr

 * You can get a copy of these files from here:
   git clone https://github.com/cbsd/cbsd_useful_stuff.git

### To obtain source tree via 'srcup':

Please copy ~cbsd/etc/defaults/srcup.conf to ~cbsd/etc/srcup.conf

Edit ~cbsd/etc/srcup.conf:

a) Uncomment GITBASE for DFLYBSD
b) change checkout_method="svn" to checkout_method="git"

## Missed tools:

mdconfig
ulimit
jail.h

