#!/bin/bash

set -eo pipefail

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

DIR="$( cd "$( dirname "$0" )" && pwd )"

# Configure
mkdir artifacts
git remote add upstream 'https://github.com/MSYS2/MINGW-packages'
git fetch --quiet upstream
# reduce time required to install packages by disabling pacman's disk space checking
sed -i 's/^CheckSpace/#CheckSpace/g' /etc/pacman.conf

pacman --noconfirm -Fy

# Enable colors
normal=$(tput sgr0)
red=$(tput setaf 1)
green=$(tput setaf 2)
cyan=$(tput setaf 6)

# Basic status function
_status() {
    local type="${1}"
    local status="${package:+${package}: }${2}"
    local items=("${@:3}")
    case "${type}" in
        failure) local -n nameref_color='red';   title='[MSYS2 CI] FAILURE:' ;;
        success) local -n nameref_color='green'; title='[MSYS2 CI] SUCCESS:' ;;
        message) local -n nameref_color='cyan';  title='[MSYS2 CI]'
    esac
    printf "\n${nameref_color}${title}${normal} ${status}\n\n"
    printf "${items:+\t%s\n}" "${items:+${items[@]}}"
}

# Run command with status
execute(){
    local status="${1}"
    local command="${2}"
    local arguments=("${@:3}")
    cd "${package:-.}"
    message "${status}"
    if [[ "${command}" != *:* ]]
        then ${command} ${arguments[@]}
        else ${command%%:*} | ${command#*:} ${arguments[@]}
    fi || failure "${status} failed"
    cd - > /dev/null
}

# Get changed packages in correct build order
list_packages() {
    # readarray doesn't work with a plain pipe
    readarray -t packages < <("$DIR/ci-get-build-order.py")
}

install_packages() {
    pacman --noprogressbar --upgrade --noconfirm *.pkg.tar.*
}

# List DLL dependencies
list_dll_deps(){
    local target="${1}"
    echo "$(tput setaf 2)MSYS2 DLL dependencies:$(tput sgr0)"
    find "$target" -regex ".*\.\(exe\|dll\)" -print0 | xargs -0 -r ldd | GREP_COLOR="1;35" grep --color=always "msys-.*\|" \
    || echo "        None"
}

list_dll_bases(){
    local target="${1}"
    echo "$(tput setaf 2)MSYS2 DLL bases:$(tput sgr0)"
    find "$target" -regex ".*\.\(exe\|dll\)" -print | rebase -iT - | GREP_COLOR="1;35" grep --color=always "msys-.*\|" \
    || echo "        None"
}

# Status functions
failure() { local status="${1}"; local items=("${@:2}"); _status failure "${status}." "${items[@]}"; exit 1; }
success() { local status="${1}"; local items=("${@:2}"); _status success "${status}." "${items[@]}"; exit 0; }
message() { local status="${1}"; local items=("${@:2}"); _status message "${status}"  "${items[@]}"; }

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

[ "${#packages[@]}" -eq 0 ] && success 'No changes in package recipes'

# Build
message 'Building packages' "${packages[@]}"

message 'Adding an empty local repository'
repo-add $PWD/artifacts/ci.db.tar.gz
sed -i '1s|^|[ci]\nServer = file://'"$PWD"'/artifacts/\nSigLevel = Never\n|' /etc/pacman.conf
pacman -Sy

# Remove git and python
pacman -R --recursive --unneeded --noconfirm --noprogressbar git python

# Install ntldd
pacman -S --needed --noconfirm ${MINGW_PACKAGE_PREFIX}-ntldd

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
                ntldd -R ${binary} | grep -v "ext-ms\|api-ms\|WINDOWS\|Windows\|HvsiFileTrust\|wpaxholder\|ngcrecovery\|AzureAttest\|PdmUtilities\|WTDSENSOR\|wtdccm\|SAMSRV\|LSASRV" || true
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
