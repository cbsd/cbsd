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
	${RM} -f bin/cbsdsh/.depend*
	${RM} -f misc/chk_arp_byip
	${RM} -f bin/cbsdsftp
	${RM} -f bin/cbsdsftp6
	${RM} -f bin/cfetch
	${RM} -f bin/cbsdssh
	${RM} -f bin/cbsdssh6
	${RM} -f bin/cbsd
	${RM} -f sbin/netmask
	${RM} -f misc/sqlcli
	${RM} -f misc/pwcrypt
	${RM} -f misc/cbsdlogtail
	${RM} -f misc/elf_tables
	${RM} -f misc/conv2human
	${RM} -f misc/cbsd_fwatch
	${RM} -f misc/popcnttest
	${RM} -f misc/cbsd_dot
	${RM} -f misc/daemon
	${RM} -f misc/resolv
	${RM} -f tools/imghelper
	${RM} -f tools/xo
	${RM} -f tools/vale-ctl
	${RM} -f tools/nic_info
	${RM} -f tools/bridge
	${RM} -f tools/racct-statsd
	${RM} -f tools/select_jail


cbsd:
	${CC} bin/cbsdsftp.c -o bin/cbsdsftp -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdsftp
	${CC} bin/cbsdsftp6.c -o bin/cbsdsftp6 -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdsftp6
	${CC} bin/cbsdssh.c -o bin/cbsdssh -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdssh
	${CC} bin/cbsdssh6.c -o bin/cbsdssh6 -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdssh6
	${CC} bin/cfetch.c -o bin/cfetch -lfetch -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cfetch
	${CC} sbin/netmask.c -o sbin/netmask && ${STRIP} sbin/netmask
	#${CC} misc/src/sqlcli.c -static -pthread -lsqlite3 -lm -L/usr/local/lib -I/usr/local/include -o misc/sqlcli && ${STRIP} misc/sqlcli
	# ICU?
	#${CC} misc/src/sqlcli.c -static -pthread -lsqlite3 -lpthread -licui18n -licuuc -licudata -lm -L/usr/local/lib -I/usr/local/include -o misc/sqlcli && ${STRIP} misc/sqlcli
	${CC} misc/src/sqlcli.c -static `pkg-config sqlite3 --cflags --libs --static` -lm -lc++ -o misc/sqlcli && ${STRIP} misc/sqlcli
	${CC} misc/src/cbsdlogtail.c -o misc/cbsdlogtail && ${STRIP} misc/cbsdlogtail
	${CC} misc/src/pwcrypt.c -lcrypt -o misc/pwcrypt && ${STRIP} misc/pwcrypt
	${CC} misc/src/chk_arp_byip.c -o misc/chk_arp_byip && ${STRIP} misc/chk_arp_byip
	${CC} misc/src/elf_tables.c -I/usr/local/include -I/usr/local/include/libelf -L/usr/local/lib -lelf -o misc/elf_tables && ${STRIP} misc/elf_tables
	${CC} misc/src/conv2human.c -I/usr/local/include -I/usr/local/include/libelf -L/usr/local/lib -lelf -o misc/conv2human -lutil && ${STRIP} misc/conv2human
	${CC} misc/src/cbsd_fwatch.c -o misc/cbsd_fwatch && ${STRIP} misc/cbsd_fwatch
	${CC} misc/src/popcnttest.c -o misc/popcnttest -msse4.2 && ${STRIP} misc/popcnttest > /dev/null 2>&1 || /usr/bin/true
	${CC} misc/src/cbsd_dot.c -o misc/cbsd_dot && ${STRIP} misc/cbsd_dot
	${CC} misc/src/daemon.c -lutil -o misc/daemon && ${STRIP} misc/daemon
	${CC} misc/src/resolv.c -o misc/resolv && ${STRIP} misc/resolv
	${CC} tools/src/imghelper.c -o tools/imghelper && ${STRIP} tools/imghelper
	${CC} tools/src/bridge.c -o tools/bridge && ${STRIP} tools/bridge
	${CC} tools/src/vale-ctl.c -o tools/vale-ctl && ${STRIP} tools/vale-ctl
	${CC} tools/src/nic_info.c -o tools/nic_info && ${STRIP} tools/nic_info
	${CC} tools/src/racct-statsd.c -lutil -lprocstat -ljail -lsqlite3 -I/usr/local/include -L/usr/local/lib -o tools/racct-statsd && ${STRIP} tools/racct-statsd
	${CC} tools/src/select_jail.c -o tools/select_jail && ${STRIP} tools/select_jail
	${MAKE} -C bin/cbsdsh && ${STRIP} bin/cbsdsh/cbsd
	${MAKE} -C share/bsdconfig/cbsd

install:
	${MKDIR} -p ${DESTDIR}${PREFIX}/cbsd
	${CP} -Rpv * ${DESTDIR}${PREFIX}/cbsd/
	${CP} -Rpv .ssh ${DESTDIR}${PREFIX}/cbsd/
	${INSTALL} man/cbsd.8 ${DESTDIR}${PREFIX}/man/man8/cbsd.8
	${ENV} BINDIR=${PREFIX}/bin ${MAKE} -C bin/cbsdsh install
	${MAKE} -C share/bsdconfig/cbsd install
