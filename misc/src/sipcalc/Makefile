PROG=		sipcalc
SRCS=		sub-func.c interface.c sipcalc.c sub-output.c
CFLAGS+=	-O2 -pipe -fstack-protector-strong -fno-strict-aliasing
MAN=
.SILENT:

clean:
	rm -f .depend* sipcalc sipcalc.debug sipcalc.full *.o
#install:
#    install -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} fscd /usr/sbin/


install:
	date

.include <bsd.prog.mk>

