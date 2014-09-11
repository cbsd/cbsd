# $FreeBSD$

NO_OBJ=

SUBDIR= include

LIBEXECDIR=	/usr/local/libexec
FILESDIR=	${LIBEXECDIR}/bsdconfig/cbsd
FILES=		INDEX USAGE

SCRIPTSDIR=	${FILESDIR}
SCRIPTS=	cbsd

beforeinstall:
	mkdir -p ${DESTDIR}${FILESDIR}

.include <bsd.prog.mk>
