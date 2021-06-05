PROG=		sipcalc
SRCS=		sub-func.c interface.c sipcalc.c sub-output.c
CFLAGS+=	-O2 -pipe -fstack-protector-strong -fno-strict-aliasing
MAN=
.SILENT:

clean:
	rm -f .depend* sipcalc sipcalc.debug sipcalc.full *.o
install:
	install -s -o root -g wheel -m 0555 sipcalc /usr/local/cbsd/misc/sipcalc

.include <bsd.prog.mk>

