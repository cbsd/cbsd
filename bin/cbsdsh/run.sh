#!/bin/sh
gmake clean
gmake
rm -f /usr/local/bin/cbsd
mv dash /usr/local/bin/cbsd

