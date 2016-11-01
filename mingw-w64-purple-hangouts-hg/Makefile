# MSYS2 makefile for Purple Hangouts

CFLAGS            := $(shell pkg-config --cflags purple libprotobuf-c json-glib-1.0) -Ipurple2compat
LIBS              := $(shell pkg-config --libs   purple libprotobuf-c json-glib-1.0) -lz
PURPLE_PLUGIN_DIR := $(shell pkg-config --variable=plugindir   purple)
PURPLE_DATA_ROOT  := $(shell pkg-config --variable=datarootdir purple)
PURPLE_PLUGIN_DIR := $(shell cygpath --unix '$(PURPLE_PLUGIN_DIR)')
PURPLE_DATA_ROOT  := $(shell cygpath --unix '$(PURPLE_DATA_ROOT)')

SOURCES = \
	hangout_media.pb-c.c \
	hangouts.pb-c.c \
	hangouts_auth.c \
	hangouts_connection.c \
	hangouts_conversation.c \
	hangouts_events.c \
	hangouts_json.c \
	hangouts_media.c \
	hangouts_pblite.c \
	libhangouts.c \
	purple2compat/http.c \
	purple2compat/purple-socket.c

.PHONY: all install clean

all: libhangouts.dll

hangouts.pb-c.c: hangouts.proto
	protoc-c --c_out=. hangouts.proto

hangout_media.pb-c.c: hangout_media.proto
	protoc-c --c_out=. hangout_media.proto

libhangouts.dll: $(SOURCES)
	gcc -Wall -shared -pipe -O2 -g -DPURPLE_PLUGINS -DENABLE_NLS $(SOURCES) $(CFLAGS) $(LIBS) -o $@

install:
	install -Dm755 libhangouts.dll $(DESTDIR)$(PURPLE_PLUGIN_DIR)/libhangouts.dll
	install -Dm644 hangouts16.png $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols/16/hangouts.png
	install -Dm644 hangouts22.png $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols/22/hangouts.png
	install -Dm644 hangouts48.png $(DESTDIR)$(PURPLE_DATA_ROOT)/pixmaps/pidgin/protocols/48/hangouts.png

clean:
	rm -f libhangouts.dll
	rm -f hangouts.pb-c.h
	rm -f hangouts.pb-c.c
	rm -f hangout_media.pb-c.h
	rm -f hangout_media.pb-c.c
