#!/bin/sh

# Since mingw-w64-pathtools does not define a package, but its source code is
# copied all over the place, it could be challenging to update the dependencees
# manually. This script should help.

die () {
	echo "$*" >&2
	exit 1
}

cd "$(dirname "$0")" || die "Could not switch to directory"

get_sha256 () {
	sha256sum.exe "$1" | sed 's/ .*//'
}

c_sum="$(get_sha256 pathtools.c)"
h_sum="$(get_sha256 pathtools.h)"

for c in ../*/pathtools.c
do
	h=${c%.c}.h

	cmp "$c" pathtools.c && cmp "$h" pathtools.h && continue

	orig_c_sum="$(get_sha256 "$c")"
	orig_h_sum="$(get_sha256 "$h")"

	dir=${c%/pathtools.c}
	pkgname=${dir#../mingw-w64-}
	pkgbuild=$dir/PKGBUILD
	grep $orig_c_sum "$pkgbuild" && grep $orig_h_sum "$pkgbuild" || die "$pkgbuild not up to date?"

	pkgrel=$(($(sed -n 's/^pkgrel=//p' "$pkgbuild")+1))

	cp -f pathtools.[ch] "$dir/" &&
	sed -i -e "s/^pkgrel=.*/pkgrel=$pkgrel/" -e "s/$orig_c_sum/$c_sum/" -e "s/$orig_h_sum/$h_sum/" "$pkgbuild" &&
	git commit -sm "$pkgname: synchronize pathtools" -- "$dir"/{PKGBUILD,pathtools.c,pathtools.h} ||
	die "Could not update $pkgname"
done
