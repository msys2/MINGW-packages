#!/usr/bin/env bash

TARGETDIR="$1"
if [ -z "$TARGETDIR" ] || ! [ -d "$(dirname "$TARGETDIR")" ] || ! mkdir "$TARGETDIR"
then
	>&2 echo "Usage:"
	>&2 echo "$0 <TARGETDIR>"
	>&2 echo "Folder for <TARGETDIR> must exist. <TARGETDIR> must not exist."
	exit 1
fi

LIBDIR="$MINGW_PREFIX/lib/notepad++"
LICENSESDIR="$MINGW_PREFIX/share/licenses/notepad++"
DOCDIR="$MINGW_PREFIX/share/doc/notepad++"
if [ -d "$LIBDIR" ] && [ -d "$LICENSESDIR" ] && [ -d "$DOCDIR" ] &&
	cp -pr "$LIBDIR"/* "$LICENSESDIR"/* "$DOCDIR"/* "$TARGETDIR"/ &&
	rm -f "$TARGETDIR"/msys2.* &&
	touch "$TARGETDIR"/doLocalConf.xml
then
	>&2 echo "Creating portable copy in '$TARGETDIR' was successful."
else
	>&2 echo "Creating portable copy in '$TARGETDIR' failed.'"
	exit 2
fi
