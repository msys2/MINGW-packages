#!/bin/bash

set -e

git config --global user.email "ci@msys2.org"
git config --global user.name "MSYS2 Build Bot"

# fetch first changed PKGBUILD file, assume at most one package touched per commit
# if you want to have your changes tested by ci then bump the pkgrel in PKGBUILD
TOUCHED=`git show -m --pretty="format:" --name-only | grep PKGBUILD | head -1`
if [ "$TOUCHED" ]
then
    PKGDIR=`dirname $TOUCHED`
    pushd $PKGDIR > /dev/null
    if [ "$RUNTEST" = "true" ]
    then
        makepkg-mingw -f -s --noconfirm --skippgpcheck || true
    else
        makepkg-mingw -f -s --noconfirm --skippgpcheck --nocheck
    fi
    popd > /dev/null
else
    echo Nothing to test
fi
