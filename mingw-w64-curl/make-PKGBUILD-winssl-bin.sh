#!/bin/sh

# The script generates PKGBUILD-winssl-bin file from base PKGBUILD file.
#
# mingw-w64-curl-winssl-bin package built from PKGBUILD-winssl-bin
# provides only binary files (.exe and .dll) in /mingw64/bin/curl-winssl.
# The package is used in Git for Windows to distribute cURL built with WinSSL
# along with cRUL built with OpenSSL which is used by default.
#
# The script uses a patch file generated with the following command:
# $ diff -u PKGBUILD PKGBUILD-winssl-bin >PKGBUILD-winssl-bin.patch

die () {
	echo "$*" >&2
	exit 1
}

patch -i PKGBUILD-winssl-bin.patch -o PKGBUILD-winssl-bin ||
die "Could not patch PKGBUILD"
