# MSYS2 makefile for Pidgin SkypeWeb

CFLAGS            := $(shell pkg-config --cflags purple json-glib-1.0)
LIBS              := $(shell pkg-config --libs   purple json-glib-1.0) -lz -lws2_32
PURPLE_PLUGIN_DIR := $(shell pkg-config --variable=plugindir   purple)
PURPLE_DATA_ROOT  := $(shell pkg-config --variable=datarootdir purple)
PURPLE_PLUGIN_DIR := $(shell cygpath --unix '$(PURPLE_PLUGIN_DIR)')
PURPLE_DATA_ROOT  := $(shell cygpath --unix '$(PURPLE_DATA_ROOT)')

SOURCES = \
	skypeweb_connection.c \
	skypeweb_contacts.c \
	skypeweb_login.c \
	skypeweb_messages.c \
	skypeweb_util.c \
	libskypeweb.c

.PHONY: all install clean

all: libskypeweb.dll

clean:
	rm -f libskypeweb.dll

libskypeweb.dll: $(SOURCES)
	gcc -Wall -shared -pipe -O2 -g -DPURPLE_PLUGINS -DENABLE_NLS -DHAVE_ZLIB $(SOURCES) $(CFLAGS) $(LIBS) -o $@

install:
	mkdir -p $(DESTDIR)$(PURPLE_PLUGIN_DIR)
	mkdir -p $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols
	mkdir -p $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/emotes/skype
	cp libskypeweb.dll $(DESTDIR)$(PURPLE_PLUGIN_DIR)/libskypeweb.dll
	cp theme $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/emotes/skype/theme
	cp -r icons/16 $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols/
	cp -r icons/22 $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols/
	cp -r icons/48 $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols/
