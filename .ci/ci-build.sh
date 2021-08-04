#!/bin/bash

set -e

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

DIR="$( cd "$( dirname "$0" )" && pwd )"

# Configure
source "$DIR/ci-library.sh"
mkdir artifacts
git_config user.email 'ci@msys2.org'
git_config user.name  'MSYS2 Continuous Integration'
git remote add upstream 'https://github.com/MSYS2/MINGW-packages'
git fetch --quiet upstream
# reduce time required to install packages by disabling pacman's disk space checking
sed -i 's/^CheckSpace/#CheckSpace/g' /etc/pacman.conf

pacman --noconfirm -Fy

# Detect
list_commits  || failure 'Could not detect added commits'
list_packages || failure 'Could not detect changed files'
message 'Processing changes' "${commits[@]}"
test -z "${packages}" && success 'No changes in package recipes'
define_build_order || failure 'Could not determine build order'

# Build
message 'Building packages' "${packages[@]}"
execute 'Approving recipe quality' check_recipe_quality

message 'Adding an empty local repository'
repo-add $PWD/artifacts/ci.db.tar.gz
sed -i '1s|^|[ci]\nServer = file://'"$PWD"'/artifacts/\nSigLevel = Never\n|' /etc/pacman.conf
pacman -Sy

message 'Building packages'
for package in "${packages[@]}"; do
    echo "::group::[build] ${package}"
    execute 'Fetch keys' "$DIR/fetch-validpgpkeys.sh"
    # Ensure the toolchain is installed before building the package
    execute 'Installing the toolchain' pacman -S --needed --noconfirm --noprogressbar ${MINGW_PACKAGE_PREFIX}-toolchain
    execute 'Building binary' makepkg-mingw --noconfirm --noprogressbar --nocheck --syncdeps --rmdeps --cleanbuild
    MINGW_ARCH=mingw64 execute 'Building source' makepkg-mingw --noconfirm --noprogressbar --allsource
    echo "::endgroup::"

    echo "::group::[install] ${package}"
    execute 'Installing' install_packages
    echo "::endgroup::"

    echo "::group::[diff] ${package}"
    cd "$package"
    for pkg in *.pkg.tar.*; do
        message "File listing diff for ${pkg}"
        pkgname="$(echo "$pkg" | rev | cut -d- -f4- | rev)"
        diff -Nur <(pacman -Fl "$pkgname" | sed -e 's|^[^ ]* |/|' | sort) <(pacman -Ql "$pkgname" | sed -e 's|^[^/]*||' | sort) || true
    done
    cd - > /dev/null
    echo "::endgroup::"

    echo "::group::[uninstall] ${package}"
    repo-add $PWD/artifacts/ci.db.tar.gz "${package}"/*.pkg.tar.*
    pacman -Sy
    cd "$package"
    for pkg in *.pkg.tar.*; do
        message "Uninstalling ${pkg}"
        pkgname="$(echo "$pkg" | rev | cut -d- -f4- | rev)"
        pacman -R --cascade --recursive --unneeded --noconfirm --noprogressbar "$pkgname"
    done
    cd - > /dev/null
    echo "::endgroup::"

    mv "${package}"/*.pkg.tar.* artifacts
    mv "${package}"/*.src.tar.* artifacts
    unset package
done
success 'All packages built successfully'

cd artifacts
execute 'SHA-256 checksums' sha256sum *
