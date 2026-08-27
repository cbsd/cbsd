PREFIX ?= /usr/local
DESTDIR ?=

NO_OBJ =

LIBEXECDIR = $(PREFIX)/libexec
FILESDIR = $(LIBEXECDIR)/bsdconfig/cbsd
FILES = INDEX USAGE

SCRIPTSDIR = $(FILESDIR)
SCRIPTS = bhyvecfg bhyvedsk bhyvenic cbsd

INSTALL ?= install
INSTALL_DATA ?= $(INSTALL) -m 644
INSTALL_SCRIPT ?= $(INSTALL) -m 755
MKDIR ?= mkdir -p

.PHONY: all install clean beforeinstall subdir-install

all: subdir-all

subdir-all:
	$(MAKE) -C include all

beforeinstall:
	$(MKDIR) $(DESTDIR)$(FILESDIR)

install: beforeinstall install-files install-scripts subdir-install

install-files: beforeinstall
	@for f in $(FILES); do \
		$(INSTALL_DATA) "$$f" "$(DESTDIR)$(FILESDIR)/$$f"; \
	done

install-scripts: beforeinstall
	@for f in $(SCRIPTS); do \
		$(INSTALL_SCRIPT) "$$f" "$(DESTDIR)$(SCRIPTSDIR)/$$f"; \
	done

subdir-install:
	$(MAKE) -C include install

clean: subdir-clean

subdir-clean:
	$(MAKE) -C include clean
