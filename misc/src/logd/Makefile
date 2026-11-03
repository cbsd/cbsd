CC?=		cc
CFLAGS?=	-Wall -Wextra -O2

.PHONY: all clean

all: cbsdlogd cbsdlogctl

cbsdlogd: cbsdlogd.c cbsdlog.h
	$(CC) $(CFLAGS) -o cbsdlogd cbsdlogd.c

cbsdlogctl: cbsdlogctl.c cbsdlog.h
	$(CC) $(CFLAGS) -o cbsdlogctl cbsdlogctl.c

clean:
	rm -f cbsdlogd cbsdlogctl
