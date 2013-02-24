# $FreeBSD:$

PORTNAME=	cbsd
PORTVERSION=	9.1.0
CATEGORIES=	sysutils
MASTER_SITES=	http://www.bsdstore.ru/downloads/

MAINTAINER=	olevole@olevole.ru
COMMENT=	Yet another FreeBSD Jail Management Utility

RUN_DEPENDS=	rsync:${PORTSDIR}/net/rsync \
		sudo:${PORTSDIR}/security/sudo \
		sqlite3:${PORTSDIR}/databases/sqlite3
LIB_DEPENDS=	ssh2:${PORTSDIR}/security/libssh2

SUB_FILES=	pkg-message
WRKSRC=		${WRKDIR}/cbsd-9.1.0/9.1.0

MAN7=		cbsd.7

.include <bsd.port.pre.mk>

.if ${OSVERSION} < 901000
BROKEN=         does not work on FreeBSD < 9.1
.endif

post-install:
	@${CAT} ${PKGMESSAGE}

.include <bsd.port.post.mk>
