# New ports collection makefile for:	cbsd
# Date created:				Jan 20, 2012
# Whom:					Oleg Ginzburg
#
# $FreeBSD: $
#

PORTNAME=	cbsd
PORTVERSION=	9.0.0
CATEGORIES=	sysutils
MASTER_SITES=	http://www.bsdstore.ru/downloads/

MAINTAINER=	olevole@olevole.ru
COMMENT=	Yet another FreeBSD Jail Management Utility

SUB_FILES=	pkg-message

.include <bsd.port.pre.mk>

post-install:
	@${CAT} ${PKGMESSAGE}

.include <bsd.port.post.mk>
