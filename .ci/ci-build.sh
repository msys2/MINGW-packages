#!/bin/bash

set -eo pipefail

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
list_packages || failure 'Could not detect changed files'
message 'Processing changes'
test -z "${packages}" && success 'No changes in package recipes'

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
    execute 'Building binary' makepkg-mingw --noconfirm --noprogressbar --nocheck --syncdeps --rmdeps --cleanbuild
    echo "::endgroup::"

    if [ -f $package/.ci-sequential ]; then
        cd "$package"
        for pkg in *.pkg.tar.*; do
            pkgname="$(echo "$pkg" | rev | cut -d- -f4- | rev)"
            echo "::group::[install] ${pkgname}"
            grep -qFx "${package}" "$(dirname "$0")/ci-dont-install-list.txt" || pacman --noprogressbar --upgrade --noconfirm $pkg
            echo "::endgroup::"

            echo "::group::[meta-diff] ${pkgname}"
            message "Package info diff for ${pkgname}"
            diff -Nur <(pacman -Si "${pkgname}") <(pacman -Qip "${pkg}") || true
            echo "::endgroup::"

            echo "::group::[file-diff] ${pkgname}"
            message "File listing diff for ${pkgname}"
            diff -Nur <(pacman -Fl "$pkgname" | sed -e 's|^[^ ]* |/|' | sort) <(pacman -Ql "$pkgname" | sed -e 's|^[^/]*||' | sort) || true
            echo "::endgroup::"

            echo "::group::[uninstall] ${pkgname}"
            message "Uninstalling $pkgname"
            repo-add $PWD/../artifacts/ci.db.tar.gz $PWD/$pkg
            pacman -Sy
            pacman -R --recursive --unneeded --noconfirm --noprogressbar "$pkgname"
            echo "::endgroup::"
        done
        cd - > /dev/null
    else
        echo "::group::[install] ${package}"
        grep -qFx "${package}" "$(dirname "$0")/ci-dont-install-list.txt" || execute 'Installing' install_packages
        echo "::endgroup::"

        echo "::group::[diff] ${package}"
        cd "$package"
        for pkg in *.pkg.tar.*; do
            pkgname="$(echo "$pkg" | rev | cut -d- -f4- | rev)"
            message "Package info diff for ${pkgname}"
            diff -Nur <(pacman -Si "${pkgname}") <(pacman -Qip "${pkg}") || true

            message "File listing diff for ${pkgname}"
            diff -Nur <(pacman -Fl "$pkgname" | sed -e 's|^[^ ]* |/|' | sort) <(pacman -Ql "$pkgname" | sed -e 's|^[^/]*||' | sort) || true
        done
        cd - > /dev/null
        echo "::endgroup::"

        echo "::group::[uninstall] ${package}"
        repo-add $PWD/artifacts/ci.db.tar.gz "${package}"/*.pkg.tar.*
        pacman -Sy
        message "Uninstalling $package"
        cd "$package"
        export installed_packages=()
        for pkg in *.pkg.tar.*; do
            installed_packages+=("$(echo "$pkg" | rev | cut -d- -f4- | rev)")
        done
        pacman -R --recursive --unneeded --noconfirm --noprogressbar "${installed_packages[@]}"
        unset installed_packages
        cd - > /dev/null
        echo "::endgroup::"
    fi

    mv "${package}"/*.pkg.tar.* artifacts
    unset package
done
success 'All packages built successfully'

cd artifacts
execute 'SHA-256 checksums' sha256sum *
