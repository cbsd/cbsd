#!/bin/sh
gmake clean
gmake
rm -f /usr/local/bin/cbsd
if [ ! -x dash ]; then
	echo
	echo "!!! ERROR !!!"
	echo
fi
mv dash /usr/local/bin/cbsd
echo "CLEAN"
make clean
gmake clean
make distclean
gmake distclean

exit 0
