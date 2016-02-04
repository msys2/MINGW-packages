#!/bin/bash

set -e

git config --global user.email "ci@msys2.org"
git config --global user.name "MSYS2 Build Bot"

# fetch first changed file, assume at most one package touched per commit
TOUCHED=`git show -m --pretty="format:" --name-only | grep . | head -1`
PKGDIR=`dirname $TOUCHED`
if [ "$PKGDIR" = "." || "$PKGDIR" = "drone" ]
then
    echo Nothing to test
else
    pushd $PKGDIR > /dev/null
    if [ "$RUNTEST" = "true" ]
    then
        makepkg-mingw -f -s --noconfirm --skippgpcheck || true
    else
        makepkg-mingw -f -s --noconfirm --skippgpcheck --nocheck
    fi
    popd > /dev/null
fi
