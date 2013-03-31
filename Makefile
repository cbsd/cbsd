PREFIX?=/usr/local

all:

install:
	mkdir -p ${PREFIX}/cbsd
	cp -Rp * ${PREFIX}/cbsd/
	cp -Rp .ssh ${PREFIX}/cbsd/
	install man7/cbsd.7  ${PREFIX}/man/man7/cbsd.7

