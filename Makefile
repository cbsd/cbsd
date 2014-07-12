PREFIX?=/usr/local
CC?=/usr/bin/cc
CBSD_HOME=${PREFIX}/cbsd

all:	cbsd

clean:
	/usr/bin/make -C bin/cbsdsh clean
	/bin/rm -f misc/chk_arp_byip
	/bin/rm -f bin/cbsdsftp
	/bin/rm -f bin/cbsdssh
	/bin/rm -f bin/cfetch
	/bin/rm -f bin/cbsd
	/bin/rm -f sbin/netmask
	/bin/rm -f misc/sqlcli
	/bin/rm -f misc/cbsdlogtail
	/bin/rm -f misc/elf_tables
	/bin/rm -f misc/conv2human
	/bin/rm -f misc/cbsd_fwatch
	/bin/rm -f misc/popcnttest

cbsd:
	${CC} bin/cbsdsftp.c -o bin/cbsdsftp -lssh2 -L/usr/local/lib -I/usr/local/include
	${CC} bin/cbsdssh.c -o bin/cbsdssh -lssh2 -L/usr/local/lib -I/usr/local/include
	${CC} bin/cfetch.c -o bin/cfetch -lfetch
	${CC} sbin/netmask.c -o sbin/netmask
	${CC} misc/src/sqlcli.c -static -lsqlite3 -L/usr/local/lib -I/usr/local/include -o misc/sqlcli
	${CC} misc/src/cbsdlogtail.c -o misc/cbsdlogtail
	${CC} misc/src/elf_tables.c -lelf -o misc/elf_tables
	${CC} misc/src/conv2human.c -lelf -o misc/conv2human -lutil
	${CC} misc/src/cbsd_fwatch.c -o misc/cbsd_fwatch
	${CC} misc/src/popcnttest.c -o misc/popcnttest -msse4.2
	/usr/bin/make -C bin/cbsdsh

install:
	mkdir -p ${DESTDIR}${PREFIX}/cbsd
	cp -Rpv * ${DESTDIR}${PREFIX}/cbsd/
	cp -Rpv .ssh ${DESTDIR}${PREFIX}/cbsd/
	install man/cbsd.8 ${DESTDIR}${PREFIX}/man/man8/cbsd.8
	/usr/bin/env BINDIR=${PREFIX}/bin /usr/bin/make -C bin/cbsdsh install
