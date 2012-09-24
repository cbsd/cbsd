# New ports collection makefile for:	cbsd
# Date created:				Jan 20, 2012
# Whom:					Oleg Ginzburg
#
# $FreeBSD: $
#

PORTNAME=	cbsd
PORTVERSION=	9.1.0
CATEGORIES=	sysutils
MASTER_SITES=	http://www.bsdstore.ru/downloads/

MAINTAINER=	olevole@olevole.ru
COMMENT=	Yet another FreeBSD Jail Management Utility

SUB_FILES=	pkg-message
WRKSRC=		${WRKDIR}/cbsd-9.1.0/9.1.0

.include <bsd.port.pre.mk>

.if ${OSVERSION} < 901500
BROKEN=         does not work on FreeBSD < 9.1
.endif

post-install:
	@${CAT} ${PKGMESSAGE}

.include <bsd.port.post.mk>
