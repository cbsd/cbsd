# PUBLIC DOMAIN - NO WARRANTY, see:
#     <http://creativecommons.org/publicdomain/zero/1.0/>

OUTPUT = bsddialog
export VERSION = 1.1.2
LIBPATH = $(CURDIR)/lib
UTILITYPATH = $(CURDIR)/utility

RM = rm -f
LN = ln -s -f

### command-line options ###
NORPATH ?=
export DISABLERPATH = $(NORPATH)
DEBUG ?=
export ENABLEDEBUG = $(DEBUG)
# Static linking: `gmake STATIC=1`
STATIC ?=
export STATIC
###################

all: $(OUTPUT)

install: all
	$(MAKE) -C $(LIBPATH) install
	$(MAKE) -C $(UTILITYPATH) install

uninstall:
	$(MAKE) -C $(UTILITYPATH) uninstall
	$(MAKE) -C $(LIBPATH) uninstall

$(OUTPUT): libbsddialog
	$(MAKE) -C $(UTILITYPATH) LIBPATH=$(LIBPATH)
	$(LN) $(UTILITYPATH)/$(OUTPUT) $(CURDIR)/$(OUTPUT)

libbsddialog:
	$(MAKE) -C $(LIBPATH)

clean:
	$(MAKE) -C $(LIBPATH) clean
	$(MAKE) -C $(UTILITYPATH) clean
	$(RM) $(OUTPUT) *.core

.PHONY: all install uninstall clean libbsddialog
