CC?= cc
OSTYPE?= uname -s
CFLAGS = -O2 -pipe  -fstack-protector-strong -fno-strict-aliasing
RM?= rm
INSTALL?= install

sipcalc: sub-func.c interface.c sipcalc.c sub-output.c
	@$(CC) $(CFLAGS) -o sipcalc sub-func.c interface.c sipcalc.c sub-output.c

clean:
	@$(RM) -f .depend* sipcalc sipcalc.debug sipcalc.full *.o
install:
	${INSTALL:} -s -o root -g wheel -m 0555 sipcalc /usr/local/cbsd/misc/sipcalc
