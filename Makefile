PREFIX?=/usr/local

all:

install:
	mkdir -p ${PREFIX}/cbsd
	cp -Rpv * ${PREFIX}/cbsd/
	cp -Rpv .ssh ${PREFIX}/cbsd/
	install man7/cbsd.7  ${PREFIX}/man/man7/cbsd.7
