PREFIX?=/usr/local

all:

install:
	mkdir -p ${PREFIX}/cbsd
	cp -Rpv * ${PREFIX}/cbsd/
	cp -Rpv .ssh ${PREFIX}/cbsd/
	install man/cbsd.8  ${PREFIX}/man/man8/cbsd.8
