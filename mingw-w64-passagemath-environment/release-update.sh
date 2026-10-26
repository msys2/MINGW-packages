#!/bin/bash

# This is a utility script which helps to update all PKGBUILD files for passagemath to a new(er)
# version. It expects the new version number as first and only parameter. Call it like
#
#    ./release-update.sh 10.8.2
#
# where 10.8.2 has to be replaced by the newer version.

if [ -z "$1" ]
then
    echo "Error: First argument must be a new version number!"
    exit 1
fi

NEW_VERSION=$1

SCRIPT_DIR=$(realpath "$(dirname "$BASH_SOURCE")")
cd "$SCRIPT_DIR/.." || exit 1

PACKAGES=$(find . -maxdepth 1 -type d -name 'mingw-w64-passagemath-*')
for pkg_dir in $PACKAGES
do
    if [ "$(basename $pkg_dir)" == "mingw-w64-passagemath-ppl" ]
    then
        echo "Skipping passagemath-ppl, because it uses a different version than the other"
        echo "passagemath packages. You might want to check that one manually."

        continue
    fi

    cd "$pkg_dir" || exit 2
    echo "Info: Updating $(basename $pkg_dir) to $NEW_VERSION ..."
    sed -i "s/^pkgver=.*/pkgver=${NEW_VERSION}/g" PKGBUILD # update version
    sed -i 's/^pkgrel=.*/pkgrel=1/' PKGBUILD # reset pkgrel to 1
    updpkgsums
    if [ $? -ne 0 ]
    then
        echo "Error: Update of checksum for package $pkg_dir with updpkgsums failed!"
        echo "Maybe it's not available on PyPI yet?"
        exit 3
    fi

    cd .. || exit 4
done

exit 0
