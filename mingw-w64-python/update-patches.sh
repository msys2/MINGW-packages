#!/bin/bash

set -e

die () {
	echo "$*" >&2
	exit 1
}

cd "$(dirname "$0")" ||
die "Could not cd to mingw-w64-python3.9/"

git rev-parse --verify HEAD >/dev/null &&
git update-index -q --ignore-submodules --refresh &&
git diff-files --quiet --ignore-submodules &&
git diff-index --cached --quiet --ignore-submodules HEAD -- ||
die "Clean worktree required"

git rm 0*.patch ||
die "Could not remove previous patches"

source PKGBUILD || die "Can't source PKGBUILD"

base_tag=refs/tags/v$pkgver
msys2_branch=mingw-v$pkgver
url=https://github.com/msys2-contrib/cpython-mingw
upstream_url=https://github.com/python/cpython

test -d cpython ||
git clone --bare $url cpython --depth=1000 --branch=$msys2_branch ||
die "Could not clone cpython"

git -C cpython fetch --no-tags $url $msys2_branch:$msys2_branch --depth=1000
git -C cpython fetch --no-tags $upstream_url $base_tag:$base_tag --depth=1000

git -c core.abbrev=7 \
	-c diff.renames=true \
	-c format.from=false \
	-c format.numbered=auto \
	-c format.useAutoBase=false \
	-C cpython \
	format-patch \
		--topo-order \
		--diff-algorithm=default \
		--no-attach \
		--no-add-header \
		--no-cover-letter \
		--no-thread \
		--suffix=.patch \
		--subject-prefix=PATCH \
		--output-directory .. \
			$base_tag..$msys2_branch ||
die "Could not generate new patch set"

patches="$(ls -1 0*.patch)" &&
for p in $patches
do
	sed -i 's/^\(Subject: \[PATCH [0-9]*\/\)[1-9][0-9]*/\1N/' $p ||
	die "Could not fix Subject: line in $p"
done &&
patches="$(ls -1 0*.patch)" &&
git add $patches ||
die "Could not stage new patch set"

in_sources="$(echo "$patches" | sed "{s/^/        /;:1;N;s/\\n/\\\\n        /;b1}")"
in_prepare="$(echo "$patches" | tr '\n' '\\' | sed -e 's/\\$//' -e 's/\\/ &&&n  /g')"
sed -i -e "/^        0.*\.patch$/{:1;N;/[^)]$/b1;s|.*|$in_sources)|}" \
	-e "/^ *apply_patch_with_msg /{:2;N;/[^}]$/b2;s|.*| apply_patch_with_msg $in_prepare\\n\\ \n  autoreconf -vfi\n}|}" \
	PKGBUILD ||
die "Could not update the patch set in PKGBUILD"

updpkgsums ||
die "Could not update the patch set checksums in PKGBUILD"

# bump pkgrel
sed "s/\(pkgrel\).*/pkgrel=$(($pkgrel + 1))/" -i PKGBUILD

git add PKGBUILD ||
die "Could not stage updates in PKGBUILD"
