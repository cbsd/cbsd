PREFIX?=/usr/local
CC?=/usr/bin/cc
CBSD_HOME=${PREFIX}/cbsd
STRIP="/usr/bin/strip"
RM="/bin/rm"
CP="/bin/cp"
MAKE="/usr/bin/make"
ENV="/usr/bin/env"
INSTALL="/usr/bin/install"
MKDIR="/bin/mkdir"

all:	cbsd

clean:
	${MAKE} -C bin/cbsdsh clean
	${RM} -f misc/chk_arp_byip
	${RM} -f bin/cbsdsftp
	${RM} -f bin/cbsdssh
	${RM} -f bin/cbsd
	${RM} -f sbin/netmask
	${RM} -f misc/sqlcli
	${RM} -f misc/cbsdlogtail
	${RM} -f misc/elf_tables
	${RM} -f misc/conv2human
	${RM} -f misc/cbsd_fwatch
	${RM} -f misc/popcnttest
	${RM} -f misc/cbsd_dot
	${RM} -f misc/daemon
	${RM} -f tools/xo


cbsd:
	${CC} bin/cbsdsftp.c -o bin/cbsdsftp -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdsftp
	${CC} bin/cbsdssh.c -o bin/cbsdssh -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdssh
	${CC} sbin/netmask.c -o sbin/netmask && ${STRIP} sbin/netmask
	${CC} misc/src/sqlcli.c -static -pthread -lsqlite3 -L/usr/local/lib -I/usr/local/include -o misc/sqlcli && ${STRIP} misc/sqlcli
	${CC} misc/src/cbsdlogtail.c -o misc/cbsdlogtail && ${STRIP} misc/cbsdlogtail
	${CC} misc/src/chk_arp_byip.c -o misc/chk_arp_byip && ${STRIP} misc/chk_arp_byip
	${CC} misc/src/elf_tables.c -I/usr/local/include -I/usr/local/include/libelf -L/usr/local/lib -lelf -o misc/elf_tables && ${STRIP} misc/elf_tables
	${CC} misc/src/conv2human.c -I/usr/local/include -I/usr/local/include/libelf -L/usr/local/lib -lelf -o misc/conv2human -lutil && ${STRIP} misc/conv2human
	${CC} misc/src/cbsd_fwatch.c -o misc/cbsd_fwatch && ${STRIP} misc/cbsd_fwatch
	${CC} misc/src/popcnttest.c -o misc/popcnttest -msse4.2 && ${STRIP} misc/popcnttest > /dev/null 2>&1 || /usr/bin/true
	${CC} misc/src/cbsd_dot.c -o misc/cbsd_dot && ${STRIP} misc/cbsd_dot
	${CC} misc/src/daemon.c -lutil -o misc/daemon && ${STRIP} misc/daemon
	${CC} tools/src/xo.c -lxo -I/usr/include/libxo -I/usr/local/include/libxo -L/usr/local/lib -lxo -o tools/xo && ${STRIP} tools/xo
	${MAKE} -C bin/cbsdsh && ${STRIP} bin/cbsdsh/cbsd
	${MAKE} -C share/bsdconfig/cbsd

install:
	${MKDIR} -p ${DESTDIR}${PREFIX}/cbsd
	${CP} -Rpv * ${DESTDIR}${PREFIX}/cbsd/
	${CP} -Rpv .ssh ${DESTDIR}${PREFIX}/cbsd/
	${INSTALL} man/cbsd.8 ${DESTDIR}${PREFIX}/man/man8/cbsd.8
	${ENV} BINDIR=${PREFIX}/bin ${MAKE} -C bin/cbsdsh install
	${MAKE} -C share/bsdconfig/cbsd install
