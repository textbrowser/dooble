UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	MAKEFILE=Makefile.osx
else ifeq ($(UNAME), FreeBSD)
	MAKEFILE=Makefile.freebsd
else ifeq ($(UNAME), Linux)
	MAKEFILE=Makefile.linux
else ifeq ($(UNAME), OS/2)
	MAKEFILE=Makefile.os2
else
	MAKEFILE=Makefile.windows
endif

all:
	$(MAKE) -f $(MAKEFILE)

clean:
	$(MAKE) -f $(MAKEFILE) clean

distclean: clean

install:
	$(MAKE) -f $(MAKEFILE) install

library:
	$(MAKE) -f $(MAKEFILE) library

purge:
	$(MAKE) -f $(MAKEFILE) purge

uninstall:
	$(MAKE) -f $(MAKEFILE) uninstall
