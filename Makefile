PREFIX?=/usr/local

all:

install:
	mkdir -p ${DESTDIR}${PREFIX}/cbsd
	cp -Rpv * ${DESTDIR}${PREFIX}/cbsd/
	cp -Rpv .ssh ${DESTDIR}${PREFIX}/cbsd/
	install man/cbsd.8 ${DESTDIR}${PREFIX}/man/man8/cbsd.8
