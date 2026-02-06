MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR  := $(dir $(MKFILE_PATH))

PREFIX := $(abspath $(MKFILE_DIR)../../..)

CC?= cc
CFLAGS = -O2 -pipe -fstack-protector-strong -fno-strict-aliasing
RM?= rm
INSTALL?= install

sipcalc: sub-func.c interface.c sipcalc.c sub-output.c
	@$(CC) $(CFLAGS) -o sipcalc sub-func.c interface.c sipcalc.c sub-output.c

clean:
	@$(RM) -f .depend* sipcalc sipcalc.debug sipcalc.full *.o

install:
	$(INSTALL) -d -m 0755 $(PREFIX)/misc
	$(INSTALL) -s -o root -g wheel -m 0555 sipcalc $(PREFIX)/misc/sipcalc
