CC?= cc
OSTYPE?= uname -s
CFLAGS = -Wall -Wextra -O3
RM?= rm
INSTALL?= install

cbsd_md5: md5.c main.c
	@$(CC) $(CFLAGS) -o cbsd_md5 md5.c main.c

clean:
	@$(RM) -f cbsd_md5

install:
	${INSTALL} -s -o root -g 0 -m 0555 cbsd_md5 /usr/local/cbsd/misc/cbsd_md5
