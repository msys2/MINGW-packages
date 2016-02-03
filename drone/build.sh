#!/bin/bash

# fetch first changed file, assume at most one package touched per commit
TOUCHED=`git show --pretty="format:" --name-only | grep . | head -1`
PKGDIR=`dirname $TOUCHED`
if [ "$PKGDIR" = "." ]
then
    echo Nothing to test
else
    pushd $PKGDIR > /dev/null
    makepkg-mingw -f -s --noconfirm --skippgpcheck --noprogressbar
    popd > /dev/null
fi
