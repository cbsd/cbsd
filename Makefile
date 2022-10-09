PREFIX?=/usr/local
CC?=/usr/bin/cc
OSTYPE?= uname -s
CBSD_HOME=${PREFIX}/cbsd
STRIP="/usr/bin/strip"
RM="/bin/rm"
CP="/bin/cp"
MAKE="/usr/bin/make"
ENV="/usr/bin/env"
INSTALL="/usr/bin/install"
MKDIR="/bin/mkdir"
SIMPLEXMLOBJECT = lib/simplexml/simplexml.o
SIMPLEXMLHEADER = lib/simplexml/simplexml.h
DUMPCPUTOPOLOGYOBJECT = misc/src/dump_cpu_topology.o
DUMPISCSIDISCOVERYOBJECT = misc/src/dump_iscsi_discovery.o
.if !defined(ARCH)
ARCH!=  uname -p
.endif
VERSION != grep myversion cbsd.conf | sed s/.*=//
BUMPVERSION = ${VERSION:S/a//}
#GIT != which git
#SED != which sed
GIT = echo git
SED = echo sed

.SILENT:

all:	cbsd dump_cpu_topology dump_iscsi_discovery

clean:
	${MAKE} -C bin/cbsdsh clean
	${MAKE} -C misc/src/sipcalc clean
	${RM} -f bin/cbsdsh/.depend* misc/src/*.o ${SIMPLEXMLOBJECT} ${DUMPCPUTOPOLOGYOBJECT} ${DUMPISCSIDISCOVERYOBJECT}

distclean:
	${MAKE} -C bin/cbsdsh clean
	${RM} -f bin/cbsdsh/.depend*
	${RM} -f misc/chk_arp_byip
	${RM} -f misc/cbsdtee
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
	${RM} -f misc/fmagic
	${RM} -f misc/conv2human
	${RM} -f misc/cbsd_fwatch
# x86_64 for DFLY
.if ${ARCH} == amd64 || ${ARCH} == i386 || ${ARCH} == x86_64
	${RM} -f misc/popcnttest
.endif
	${RM} -f misc/cbsd_dot
	${RM} -f misc/daemon
	${RM} -f misc/resolv
	${RM} -f misc/ipv6range
.if ${OSTYPE} != DragonFly
	${RM} -f misc/next-vale-port
	${RM} -f tools/vale-ctl
.endif
	${RM} -f tools/imghelper
	${RM} -f tools/xo
	${RM} -f tools/nic_info
	${RM} -f tools/bridge
	${RM} -f tools/racct-jail-statsd
	${RM} -f tools/racct-bhyve-statsd
	${RM} -f tools/racct-hoster-statsd
	${RM} -f tools/select_jail
	${RM} -f misc/sipcalc
	# clean object files
	${RM} -f misc/dump_cpu_topology
	${RM} -f misc/dump_iscsi_discovery
	${RM} -f ${SIMPLEXMLOBJECT}
	${RM} -f ${DUMPCPUTOPOLOGYOBJECT}
	${RM} -f ${DUMPISCSIDISCOVERYOBJECT}

dump_cpu_topology:
	${CC} -g -c -Wall -Ilib/simplexml misc/src/dump_cpu_topology.c -o ${DUMPCPUTOPOLOGYOBJECT}
	${CC} -g -c -Wall -Ilib/simplexml lib/simplexml/simplexml.c -o ${SIMPLEXMLOBJECT}
	${CC} -g -o misc/dump_cpu_topology ${DUMPCPUTOPOLOGYOBJECT} ${SIMPLEXMLOBJECT}
	${STRIP} misc/dump_cpu_topology

dump_iscsi_discovery:
	${CC} -g -c -Wall -Ilib/simplexml misc/src/dump_iscsi_discovery.c -o ${DUMPISCSIDISCOVERYOBJECT}
	${CC} -g -c -Wall -Ilib/simplexml lib/simplexml/simplexml.c -o ${SIMPLEXMLOBJECT}
	${CC} -g -o misc/dump_iscsi_discovery ${DUMPISCSIDISCOVERYOBJECT} ${SIMPLEXMLOBJECT}
	${STRIP} misc/dump_iscsi_discovery

pkg-config-check:
	@/usr/bin/which -s pkg-config || \
		(echo "pkg-config must be present on the system to build CBSD from the source. Please install it first: pkg install pkgconf"; /usr/bin/false)

cbsd: pkg-config-check
	${CC} bin/src/cbsdsftp.c -o bin/cbsdsftp -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdsftp
	${CC} bin/src/cbsdsftp6.c -o bin/cbsdsftp6 -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdsftp6
	${CC} bin/src/cbsdssh.c -o bin/cbsdssh -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdssh
	${CC} bin/src/cbsdssh6.c -o bin/cbsdssh6 -lssh2 -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cbsdssh6
	${CC} bin/src/cfetch.c -o bin/cfetch -lfetch -L/usr/local/lib -I/usr/local/include && ${STRIP} bin/cfetch
	${CC} sbin/src/netmask.c -o sbin/netmask && ${STRIP} sbin/netmask
	${CC} misc/src/sqlcli.c `pkg-config sqlite3 --cflags --libs` -lm -o misc/sqlcli && ${STRIP} misc/sqlcli
	${CC} misc/src/cbsdlogtail.c -o misc/cbsdlogtail && ${STRIP} misc/cbsdlogtail
	${CC} misc/src/pwcrypt.c -lcrypt -o misc/pwcrypt && ${STRIP} misc/pwcrypt
	${CC} misc/src/chk_arp_byip.c -o misc/chk_arp_byip && ${STRIP} misc/chk_arp_byip
	${CC} misc/src/cbsdtee.c -o misc/cbsdtee && ${STRIP} misc/cbsdtee
	${CC} misc/src/elf_tables.c -I/usr/local/include -I/usr/local/include/libelf -L/usr/local/lib -lelf -o misc/elf_tables && ${STRIP} misc/elf_tables
	${CC} misc/src/fmagic.c -lmagic -o misc/fmagic && ${STRIP} misc/fmagic
	${CC} misc/src/conv2human.c -I/usr/local/include -I/usr/local/include/libelf -L/usr/local/lib -lelf -o misc/conv2human -lutil && ${STRIP} misc/conv2human
	${CC} misc/src/cbsd_fwatch.c -o misc/cbsd_fwatch && ${STRIP} misc/cbsd_fwatch
# x86_64 for DFLY
.if ${ARCH} == amd64 || ${ARCH} == i386 || ${ARCH} == x86_64
	${CC} misc/src/popcnttest.c -o misc/popcnttest -msse4.2 && ${STRIP} misc/popcnttest > /dev/null 2>&1 || /usr/bin/true
.endif
	${CC} misc/src/cbsd_dot.c -o misc/cbsd_dot && ${STRIP} misc/cbsd_dot
	${CC} misc/src/daemon.c -lutil -o misc/daemon && ${STRIP} misc/daemon
	${CC} misc/src/resolv.c -o misc/resolv && ${STRIP} misc/resolv
	${CC} misc/src/ipv6range.c -o misc/ipv6range && ${STRIP} misc/ipv6range
.if ${OSTYPE} != DragonFly
	${CC} misc/src/next-vale-port.c -o misc/next-vale-port && ${STRIP} misc/next-vale-port
	${CC} tools/src/vale-ctl.c -o tools/vale-ctl && ${STRIP} tools/vale-ctl
	${CC} tools/src/bridge.c -o tools/bridge && ${STRIP} tools/bridge
.endif
	${CC} tools/src/imghelper.c -o tools/imghelper && ${STRIP} tools/imghelper
	${CC} tools/src/nic_info.c -o tools/nic_info && ${STRIP} tools/nic_info

.if defined(WITH_INFLUX)
	EXTRAC=" ../../bin/cbsdsh/contrib/ini.c -lcurl -DWITH_INFLUX"
.endif

.if ${OSTYPE} != DragonFly
	${CC} tools/src/racct-jail-statsd.c lib/beanstalk-client/beanstalk.c ${EXTRAC} -lutil -lpthread -lprocstat -ljail -lsqlite3 -I/usr/local/include -Ilib/beanstalk-client -L/usr/local/lib -o tools/racct-jail-statsd && ${STRIP} tools/racct-jail-statsd
	${CC} tools/src/racct-bhyve-statsd.c lib/beanstalk-client/beanstalk.c  ${EXTRAC} -lutil -lprocstat -ljail -lsqlite3 -I/usr/local/include -Ilib/beanstalk-client -L/usr/local/lib -o tools/racct-bhyve-statsd && ${STRIP} tools/racct-bhyve-statsd
	${CC} tools/src/racct-hoster-statsd.c lib/beanstalk-client/beanstalk.c ${EXTRAC} -lutil -lprocstat -ljail -lsqlite3 -lpthread -I/usr/local/include -Ilib/beanstalk-client -L/usr/local/lib -o tools/racct-hoster-statsd && ${STRIP} tools/racct-hoster-statsd
.endif

.if defined(WITH_REDIS)
	EXTRAC+=" ../../bin/cbsdsh/cbsdredis.c ../../bin/cbsdsh/contrib/credis.c -DWITH_REDIS"
.endif
	${CC} tools/src/select_jail.c -o tools/select_jail && ${STRIP} tools/select_jail
	${MAKE} -C bin/cbsdsh && ${STRIP} bin/cbsdsh/cbsd
	${MAKE} -C misc/src/sipcalc && ${STRIP} misc/src/sipcalc/sipcalc
	${MAKE} -C share/bsdconfig/cbsd

install:
	${INSTALL} man/cbsd.8 ${DESTDIR}${PREFIX}/man/man8/cbsd.8
	${INSTALL} -o cbsd -g cbsd -m 555 misc/src/sipcalc/sipcalc ${PREFIX}/cbsd/misc/sipcalc
	${ENV} BINDIR=${PREFIX}/bin ${MAKE} -C bin/cbsdsh install
	${MAKE} -C share/bsdconfig/cbsd install

bump:
# check if version has "a" postfix
.ifdef ${VERSION:M"*a"}
	# change version in files
	${SED} -i '' "s/myversion.*/myversion=\"${BUMPVERSION}\"/" cbsd.conf
	${SED} -i '' "s/VERSION.*/VERSION \"${BUMPVERSION}\"/" bin/cbsdsh/about.h
	${GIT} add cbsd.conf bin/cbsdsh/about.h
	${GIT} commit -m \"${BUMPVERSION}\"
	# stuff from https://redmine.convectix.com/projects/cloud/wiki/Cbsd_git_github
	${GIT} checkout -b \"${BUMPVERSION}\"
	${GIT} push --set-upstream origin ${BUMPVERSION}
	${GIT} tag -a \"v${BUMPVERSION}\" -m \"${BUMPVERSION} release\"
	${GIT} push origin --tags
.endif
.ifdef NEWVERSION
	${GIT} checkout develop
	${SED} -i '' "s/myversion.*/myversion=\"${NEWVERSION}a\"/" cbsd.conf
	${SED} -i '' "s/VERSION.*/VERSION \"${NEWVERSION}a\"/" bin/cbsdsh/about.h
	${GIT} commit -am \"The Show Must Go On\"
.endif

test:
	cd tests && ./runall
