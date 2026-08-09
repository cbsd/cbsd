# PUBLIC DOMAIN - NO WARRANTY, see:
#     <http://creativecommons.org/publicdomain/zero/1.0/>
#
# Written in 2023 by Alfonso Sabato Siciliano

OUTPUT = bsddialog
export VERSION=1.1.2
.CURDIR ?= ${CURDIR}
LIBPATH = ${.CURDIR}/lib
LIBBSDDIALOG = ${LIBPATH}/libbsddialog.so
UTILITYPATH = ${.CURDIR}/utility

RM= rm -f
LN = ln -s -f

### command-line options ###
# Library run time search path, for system lib path (for package creation):
#  FreeBSD port Makefile 'MAKE_ARGS = -DNORPATH' or Linux `gmake NORPATH=1`
# otherwise the utility looks for the library in <gitrepo>/lib.
NORPATH ?=
export DISABLERPATH=${NORPATH}
# Debug: `make -DDEBUG` or `gmake DEBUG=1`
DEBUG ?=
export ENABLEDEBUG=${DEBUG}
###################

all: ${OUTPUT}

install: all
	${MAKE} -C ${LIBPATH} install
	${MAKE} -C ${UTILITYPATH} install

uninstall:
	${MAKE} -C ${UTILITYPATH} uninstall
	${MAKE} -C ${LIBPATH} uninstall

${OUTPUT}: ${LIBBSDDIALOG}
	${MAKE} -C ${UTILITYPATH} LIBPATH=${LIBPATH}
	${LN} ${UTILITYPATH}/${OUTPUT} ${.CURDIR}/${OUTPUT}

${LIBBSDDIALOG}:
	${MAKE} -C ${LIBPATH}

clean:
	${MAKE} -C ${LIBPATH} clean
	${MAKE} -C ${UTILITYPATH} clean
	${RM} ${OUTPUT} *.core

.PHONY: all install uninstall clean
