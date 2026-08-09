# PUBLIC DOMAIN - NO WARRANTY, see:
#     <http://creativecommons.org/publicdomain/zero/1.0/>
#
# Written in 2021 by Alfonso Sabato Siciliano

OUTPUT =  bsddialog
SOURCES = bsddialog.c util_builders.c util_cli.c util_theme.c
OBJECTS = $(SOURCES:.c=.o)
PREFIX ?= /usr/local
LIBPATH ?= ../lib

ifneq ($(ENABLEDEBUG),)
CFLAGS += -g
endif
CFLAGS += -D_GNU_SOURCE -Wall -Wextra -I$(LIBPATH)

STATIC ?=
ifneq ($(STATIC),)
LDFLAGS += $(LIBPATH)/libbsddialog.a -ltinfo -lncursesw
else
ifneq ($(DISABLERPATH),)
LDFLAGS += -ltinfo -L$(LIBPATH) -lbsddialog
else
LDFLAGS += -ltinfo -Wl,-rpath=$(LIBPATH) -L$(LIBPATH) -lbsddialog
endif
endif

INSTALL = install
RM = rm -f

all: $(OUTPUT)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m 0755 $(OUTPUT) $(DESTDIR)$(PREFIX)/bin/$(OUTPUT)
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	$(INSTALL) -m 0644 bsddialog.1 $(DESTDIR)$(PREFIX)/share/man/man1/bsddialog.1

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(OUTPUT)
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man1/bsddialog.1

$(OUTPUT): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OUTPUT) *.o *~

.PHONY: all install uninstall clean
