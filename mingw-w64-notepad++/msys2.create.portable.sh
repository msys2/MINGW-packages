#!/usr/bin/env bash

SCRIPT="$(realpath $0)"
TARGETDIR="$1"

LIBDIR="$MINGW_PREFIX/lib/notepad++"
LICENSESDIR="$MINGW_PREFIX/share/licenses/notepad++"
DOCDIR="$MINGW_PREFIX/share/doc/notepad++"

function fail {
	local MSG="$1"
	>&2 echo "Usage:"
	>&2 echo "$SCRIPT <TARGETDIR>"
	>&2 echo "$MSG"
	exit 1
}

if [ "$SCRIPT" != "$DOCDIR/$(basename $SCRIPT)" ]
then
	fail "$SCRIPT needs to be executed in its own environment."
fi
if [ -z "$TARGETDIR" ]
then
	fail "Missing <TARGETDIR>."
fi
if ! [ -d "$(dirname "$TARGETDIR")" ]
then
	fail "Parent folder of $TARGETDIR must exist."
fi
if ! mkdir -p "$TARGETDIR"
then
	fail "Couldn't create '$TARGETDIR'."
fi

if [ -d "$LIBDIR" ] && [ -d "$LICENSESDIR" ] && [ -d "$DOCDIR" ] &&
	touch "$TARGETDIR"/doLocalConf.xml &&
	cp -pr "$LIBDIR"/* "$LICENSESDIR"/* "$DOCDIR"/* "$TARGETDIR"/ &&
	rm -f "$TARGETDIR"/msys2.*
then
	>&2 echo "Creating portable copy in '$TARGETDIR' was successful."
else
	fail "Creating portable copy in '$TARGETDIR' failed."
fi
