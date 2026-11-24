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
normal=$(tput sgr0 2>/dev/null || true)
red=$(tput setaf 1 2>/dev/null || true)
green=$(tput setaf 2 2>/dev/null || true)
cyan=$(tput setaf 6 2>/dev/null || true)

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
    # format-string safety
    printf '\n%s%s%s %s\n\n' "${nameref_color}" "${title}" "${normal}" "${status}"
    # if an array has an empty element, it will print nothing after it
    if [ "${#items[@]}" -gt 0 ]; then
        printf '\t%s\n' "${items[@]}"
    fi
}

# Run command with status
execute(){
    local status="${1}"
    local command="${2}"
    local arguments=("${@:3}")
    cd "${package:-.}"
    message "${status}"
    if [[ "${command}" != *:* ]]
        then "${command}" "${arguments[@]}"
        else "${command%%:*}" | "${command#*:}" "${arguments[@]}"
    fi || failure "${status} failed"
    cd - > /dev/null
}

# Get changed packages in correct build order
list_packages() {
    # readarray doesn't work with a plain pipe
    readarray -t packages < <("$DIR/ci-get-build-order.py")

    # readarray returns its own exit status, if ci-get-build-order.py crashes
    # and outputs nothing, list_packages still succeeds and reports "no changes"
    # code below propagates exit status of the most recent process substitution ($! is a PID)
    wait "$!"
}

pkgver_from_metadata() {
    local version
    version="$(sed -n 's/^Version[[:space:]]*:[[:space:]]*//p')"
    version="${version%-*}"
    # escape all metacharacters
    printf '%s\n' "${version#*~}" | sed 's/[][\.^$*|]/\\&/g'
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
        if [[ "${packages[i]}" == "${package}" ]]; then
            # unquoted undergoes pathname expansion
            unset 'packages[i]'
        fi
    done
    unset package
done

[ "${#packages[@]}" -eq 0 ] && success 'No changes in package recipes'

# Build
message 'Building packages' "${packages[@]}"

message 'Adding an empty local repository'
repo-add "$PWD/artifacts/ci.db.tar.gz"
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
    rm -rf B && cp -r "${package}" B && cd B
    message 'Building binary'
    makepkg-mingw --noconfirm --noprogressbar --nocheck --syncdeps --rmdeps --cleanbuild || failure 'Building binary failed'
    cd - > /dev/null
    repo-add "$PWD/artifacts/ci.db.tar.gz" "$PWD/B"/*.pkg.tar.*
    pacman -Sy
    cp "$PWD/B"/*.pkg.tar.* "$PWD/artifacts"
    echo "::endgroup::"

    cd B
    for pkg in *.pkg.tar.*; do
        # skip signatures as they cannot be build
        [[ "$pkg" == *.sig ]] && continue

        pkgname="$(echo "$pkg" | rev | cut -d- -f4- | rev)"
        skip_install=0
        grep -qFx "${package}" "$DIR/ci-dont-install-list.txt" && skip_install=1

        echo "::group::[install] ${pkgname}"
        [ "${skip_install}" -eq 1 ] || pacman --noprogressbar --upgrade --noconfirm "$pkg"
        echo "::endgroup::"

        echo "::group::[meta-diff] ${pkgname}"
        message "Package info diff for ${pkgname}"
        diff -Nur <(pacman -Si ${MSYSTEM,,}/"${pkgname}" | sed '/Download Size/d') <(pacman -Qip "${pkg}" | sed '/Compressed Size/d') || true
        echo "::endgroup::"

        echo "::group::[file-diff] ${pkgname}"
        message "File listing diff for ${pkgname}"
        old_pkgver="$(pacman -Si ${MSYSTEM,,}/"${pkgname}" | pkgver_from_metadata || true)"
        new_pkgver="$(pacman -Qip "${pkg}" | pkgver_from_metadata || true)"
        diff -Nur \
            <(pacman -Fl ${MSYSTEM,,}/"${pkgname}" | sed -e 's|^[^ ]* |/|' ${old_pkgver:+-e "s|${old_pkgver}|PKGVER|g"} | sort) \
            <(pacman -Qlp "${pkg}" | sed -e 's|^[^ ]* ||' ${new_pkgver:+-e "s|${new_pkgver}|PKGVER|g"} | sort) || true
        echo "::endgroup::"

        echo "::group::[runtime-dependencies] ${pkgname}"
        if [ "${skip_install}" -eq 0 ]; then
            message "Runtime dependencies for ${pkgname}"
            readarray -t binaries < <(pacman -Ql "${pkgname}" | sed -e 's|^[^ ]* ||' | grep -E "${MINGW_PREFIX}/.+\.(dll|exe|pyd)$")
            if [ "${#binaries[@]}" -ne 0 ]; then
                for binary in "${binaries[@]}"; do
                    echo "${binary}:"
                    ntldd -R "${binary}" | grep -v "ext-ms\|api-ms\|WINDOWS\|Windows\|HvsiFileTrust\|wpaxholder\|ngcrecovery\|AzureAttest\|PdmUtilities\|WTDSENSOR\|wtdccm\|SAMSRV\|LSASRV\|EDPCSP\|policymanagerprecheck\|dmenterprisediagnostics\|EFSCORE\|FVESKYBACKUP\|DeclaredConfiguration" || true
                done
            fi
        fi
        echo "::endgroup::"

        echo "::group::[uninstall] ${pkgname}"
        message "Uninstalling $pkgname"
        [ "${skip_install}" -eq 1 ] || pacman -R --recursive --unneeded --noconfirm --noprogressbar "$pkgname"
        echo "::endgroup::"
    done
    cd - > /dev/null

    rm -rf B
    unset package
done
success 'All packages built successfully'
