SONAME_MAJOR=5
SONAME_MINOR=6
LIBNAME=libmongoose

SHARED_LIB=$(LIBNAME)-$(SONAME_MAJOR).dll
STATIC_LIB=$(LIBNAME).a
IMPORT_LIB=$(LIBNAME).dll.a

ifndef CC
  CC=gcc
endif

ifndef AR
  AR=ar
endif

ifndef PREFIX
  PREFIX=/usr/local
endif

ifndef BINDIR
  BINDIR=$(PREFIX)/bin
endif

ifndef LIBDIR
  LIBDIR=$(PREFIX)/lib
endif

ifndef DOCDIR
  DOCDIR=$(PREFIX)/share/$(LIBNAME)/doc
endif

CPPFLAGS+= #-DNS_ENABLE_SSL -DNS_ENABLE_IPV6
CFLAGS+=-c -Wall
LDFLAGS+=-shared -Wl,--out-implib,$(IMPORT_LIB) -lws2_32 #-lssl

SRCS=mongoose.c
OBJS=mongoose.o
INCLUDE=.

all: $(STATIC_LIB) $(SHARED_LIB)

$(SHARED_LIB): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(SHARED_LIB)

$(STATIC_LIB): $(OBJS)
	$(AR) rcs $(STATIC_LIB) $(OBJS)

$(OBJS): $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(INCLUDE) $(SRCS) -o $(OBJS)

install: $(SHARED_LIB)
	install -Dm655 $(SHARED_LIB) $(DESTDIR)$(BINDIR)/$(SHARED_LIB)
	install -Dm644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	install -Dm644 $(IMPORT_LIB) $(DESTDIR)$(LIBDIR)/$(IMPORT_LIB)
	install -Dm644 mongoose.h $(DESTDIR)$(PREFIX)/include/mongoose.h
	install -dm755 $(DESTDIR)$(DOCDIR)
	cp -a examples $(DESTDIR)$(DOCDIR)

test:
	$(MAKE) -C test/
clean:
	rm -f $(OBJS) $(SHARED_LIB) $(STATIC_LIB)
	$(MAKE) -C test/ clean

.PHONY: clean test all
