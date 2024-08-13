PROG=	cbsd
BINDIR?=	/usr/local/bin
OSTYPE?= uname -s
INSTALLFLAGS=	-S -m 500
SHSRCS=	about.c alias.c arith_yacc.c arith_yylex.c cbsdhistory.c cbsd_fwatch.c cbsd_pwait.c cd.c echo.c \
	error.c eval.c exec.c expand.c histedit.c input.c jobs.c kill.c logger.c main.c memalloc.c miscbltin.c \
	mystring.c options.c output.c parser.c printf.c redir.c show.c sqlcmd.c spawn_task.c \
	test.c trap.c var.c

.SILENT:

# HBSD/FBSD
SHSRCS+=	jail.c

.if defined(WITH_LUA)
SHSRCS+=	cbsdlua.c
CFLAGS+=	-I/usr/local/include/lua52 -l lua-5.2 -DLUA
.endif

.if defined(WITH_REDIS) || defined(WITH_DBI) || defined(WITH_INFLUX)
SHSRCS+=	contrib/ini.c cbsdcmd.c
.endif

.if defined(WITH_REDIS)
SHSRCS+=	cbsdredis.c contrib/credis.c
CFLAGS+=	-DWITH_REDIS 
.endif

.if defined(WITH_INFLUX)
SHSRCS+=	cbsdinflux.c
CFLAGS+=	-DWITH_INFLUX -lcurl
.endif

.if defined(WITH_DBI)
CFLAGS+=	-DWITH_DBI
.endif

GENSRCS= builtins.c nodes.c syntax.c
GENHDRS= builtins.h nodes.h syntax.h token.h
SRCS= ${SHSRCS} ${GENSRCS} ${GENHDRS}

# MLINKS for Shell built in commands for which there are no userland
# utilities of the same name are handled with the associated manpage,
# builtin.1 in share/man/man1/.
WITHOUT_MAN=	yes
NOMAN=		yes

DPADD= ${LIBEDIT} ${LIBTERMCAP}
# HBSD/FBSD but not for DFLY

.if ${OSTYPE} == DragonFly
LDADD= -ledit -lm -pthread `pkg-config sqlite3 --libs` -lm
. else
LDADD= -ledit -ltermcap -lm -pthread -ljail `pkg-config sqlite3 --libs` -lm
.endif
#LDADD= -ledit -lm -pthread `pkg-config sqlite3 --libs` -lm

#CFLAGS+= -DSHELL -I. -I${.CURDIR} -DCBSD -I/usr/local/include -L/usr/lib -L/usr/local/lib
CFLAGS+= -Wall -DSHELL -I. -I${.CURDIR} -DCBSD -I/usr/local/include -L/usr/local/lib


# for debug:
#DEBUG_FLAGS+= -g -DDEBUG=2 -fno-inline

.PATH:	${.CURDIR}/bltin \
	${.CURDIR}/kill \
	${.CURDIR}/test \
	${.CURDIR}/printf

CLEANFILES+= mknodes mknodes.o builtins.def.o \
	mksyntax mksyntax.o
CLEANFILES+= ${GENSRCS} ${GENHDRS} .depend*

builtins.def.o:
	cp	builtins.def builtins.def.o
.if defined(WITH_REDIS)
	echo 	" -- You are building CBSDSH with Redis; This is an experimental feature however it should be stable!"
	cat	builtins.redis >> builtins.def.o
.endif
.if defined(WITH_INFLUX)
	echo	" -- You are building CBSDSH with Influx; This is an experimental feature!"
	cat	builtins.influx >> builtins.def.o
.endif
.if defined(WITH_DBI)
	echo	" -- You are building CBSDSH with DBI; This is experimental and could be unstable!"
.endif


build-tools: mknodes mksyntax

.ORDER: builtins.c builtins.h
builtins.c builtins.h: mkbuiltins builtins.def.o
	sh ${.CURDIR}/mkbuiltins ${.CURDIR}

# XXX this is just to stop the default .c rule being used, so that the
# intermediate object has a fixed name.
# XXX we have a default .c rule, but no default .o rule.
.o:
	${CC} ${CFLAGS} ${LDFLAGS} ${.IMPSRC} ${LDLIBS} -o ${.TARGET} 
mknodes: mknodes.o
mksyntax: mksyntax.o

.ORDER: nodes.c nodes.h
nodes.c nodes.h: mknodes nodetypes nodes.c.pat
	./mknodes ${.CURDIR}/nodetypes ${.CURDIR}/nodes.c.pat

.ORDER: syntax.c syntax.h
syntax.c syntax.h: mksyntax
	./mksyntax

token.h: mktokens-cbsd
	sh ${.CURDIR}/mktokens-cbsd

regress:
	cd ${.CURDIR}/../../tools/regression/bin/sh && ${MAKE} SH=${.OBJDIR}/sh

.include <bsd.prog.mk>
