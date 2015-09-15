# $FreeBSD$

NO_OBJ=

SUBDIR= include

LIBEXECDIR=	/usr/local/libexec
FILESDIR=	${LIBEXECDIR}/bsdconfig/wbinstall
FILES=		INDEX USAGE

SCRIPTSDIR=	${FILESDIR}
SCRIPTS=	wbinstall install selectdsk

beforeinstall:
	mkdir -p ${DESTDIR}${FILESDIR}

.include <bsd.prog.mk>
