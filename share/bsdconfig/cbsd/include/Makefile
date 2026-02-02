# Converted from FreeBSD bmake for GNU make

PREFIX ?= /usr/local
DESTDIR ?=

NO_OBJ =

LIBEXECDIR = $(PREFIX)/libexec
FILESDIR = $(LIBEXECDIR)/bsdconfig/cbsd/include
FILES = messages.subr

INSTALL ?= install
INSTALL_DATA ?= $(INSTALL) -m 644
MKDIR ?= mkdir -p

.PHONY: all install clean beforeinstall

all:

beforeinstall:
	$(MKDIR) $(DESTDIR)$(FILESDIR)

install: beforeinstall
	@for f in $(FILES); do \
		$(INSTALL_DATA) "$$f" "$(DESTDIR)$(FILESDIR)/$$f"; \
	done

clean:
