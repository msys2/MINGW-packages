#!/bin/bash

set -eo pipefail

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

DIR="$( cd "$( dirname "$0" )" && pwd )"

# Configure
source "$DIR/ci-library.sh"
mkdir artifacts
git remote add upstream 'https://github.com/MSYS2/MINGW-packages'
git fetch --quiet upstream
# reduce time required to install packages by disabling pacman's disk space checking
sed -i 's/^CheckSpace/#CheckSpace/g' /etc/pacman.conf

pacman --noconfirm -Fy

# Detect
list_packages || failure 'Could not detect changed files'
message 'Processing changes'

declare -a skipped_packages=()
for package in "${packages[@]}"; do
    cd "${package}"
    readarray -d $'\0' -t mingw_arch < <(\
        set +eo pipefail
        mingw_arch=("mingw32" "mingw64" "ucrt64" "clang64")
        . PKGBUILD
        [[ "${#mingw_arch[@]}" -gt 0 ]] && printf "%s\0" "${mingw_arch[@]}"
    )
    if [[ ! " ${mingw_arch[*]} " =~ " ${MSYSTEM,,} " ]]; then
        skipped_packages+=("${package}")
    fi
    cd - > /dev/null
    unset package
done

for package in "${skipped_packages[@]}"; do
    for i in "${!packages[@]}"; do
        if [[ ${packages[i]} == $package ]]; then
            unset packages[i]
        fi
    done
    unset package
done

test -z "${packages[@]}" && success 'No changes in package recipes'

# Build
message 'Building packages' "${packages[@]}"
execute 'Approving recipe quality' check_recipe_quality

message 'Adding an empty local repository'
repo-add $PWD/artifacts/ci.db.tar.gz
sed -i '1s|^|[ci]\nServer = file://'"$PWD"'/artifacts/\nSigLevel = Never\n|' /etc/pacman.conf
pacman -Sy

# Remove git and python
pacman -R --recursive --unneeded --noconfirm --noprogressbar git python

# Enable linting
export MAKEPKG_LINT_PKGBUILD=1

message 'Building packages'
for package in "${packages[@]}"; do
    echo "::group::[build] ${package}"
    execute 'Clear cache' pacman -Scc --noconfirm
    execute 'Fetch keys' "$DIR/fetch-validpgpkeys.sh"
    cp -r ${package} B && cd B
    message 'Building binary'
    makepkg-mingw --noconfirm --noprogressbar --nocheck --syncdeps --rmdeps --cleanbuild || failure "${status} failed"
    cd - > /dev/null
    repo-add $PWD/artifacts/ci.db.tar.gz $PWD/B/*.pkg.tar.*
    pacman -Sy
    cp $PWD/B/*.pkg.tar.* $PWD/artifacts
    echo "::endgroup::"

    cd B
    for pkg in *.pkg.tar.*; do
        pkgname="$(echo "$pkg" | rev | cut -d- -f4- | rev)"
        echo "::group::[install] ${pkgname}"
        grep -qFx "${package}" "$DIR/ci-dont-install-list.txt" || pacman --noprogressbar --upgrade --noconfirm $pkg
        echo "::endgroup::"

        echo "::group::[meta-diff] ${pkgname}"
        message "Package info diff for ${pkgname}"
        diff -Nur <(pacman -Si ${MSYSTEM,,}/"${pkgname}") <(pacman -Qip "${pkg}") || true
        echo "::endgroup::"

        echo "::group::[file-diff] ${pkgname}"
        message "File listing diff for ${pkgname}"
        diff -Nur <(pacman -Fl ${MSYSTEM,,}/"$pkgname" | sed -e 's|^[^ ]* |/|' | sort) <(pacman -Ql "$pkgname" | sed -e 's|^[^/]*||' | sort) || true
        echo "::endgroup::"

        echo "::group::[runtime-dependencies] ${pkgname}"
        message "Runtime dependencies for ${pkgname}"
        declare -a binaries=($(pacman -Ql $pkgname | sed -e 's|^[^ ]* ||' | grep -E ${MINGW_PREFIX}/.+\.\(dll\|exe\|pyd\)$))
        if [ "${#binaries[@]}" -ne 0 ]; then
            for binary in ${binaries[@]}; do
                echo "${binary}:"
                ntldd -R ${binary} | grep -v "ext-ms\|api-ms\|WINDOWS\|Windows\|HvsiFileTrust\|wpaxholder\|ngcrecovery" || true
            done
        fi
        echo "::endgroup::"

        echo "::group::[uninstall] ${pkgname}"
        message "Uninstalling $pkgname"
        pacman -R --recursive --unneeded --noconfirm --noprogressbar "$pkgname"
        echo "::endgroup::"
    done
    cd - > /dev/null

    rm -rf B
    unset package
done
success 'All packages built successfully'

cd artifacts
execute 'SHA-256 checksums' sha256sum *
