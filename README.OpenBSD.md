### To get CBSD work in OpenBSD platform, you need to install extra packages:

% pkg_add flock pkgconf

Also please get next files from the bsdconfig distribution:

/usr/libexec/bsdconfig/include/messages.subr
/usr/share/bsdconfig/common.subr
/usr/share/bsdconfig/dialog.subr
/usr/share/bsdconfig/mustberoot.subr
/usr/share/bsdconfig/strings.subr
/usr/share/bsdconfig/struct.subr
/usr/share/bsdconfig/variable.subr

 * You can get a copy of these files from here:

   git clone https://github.com/cbsd/cbsd_useful_stuff.git /tmp/cbsd_useful_stuff
   cp -a /tmp/cbsd_useful_stuff/dflybsd/usr/libexec/bsdconfig /usr/libexec/
   cp -a /tmp/cbsd_useful_stuff/dflybsd/usr/share/bsdconfig /usr/share/

## Missed tools:

mdconfig
ulimit
jail.h

