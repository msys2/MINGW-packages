#!/bin/bash

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

# Functions
if [[ -t 1 ]]; then
    normal='\e[0m'
    cyan='\e[1;36m'
    green='\e[1;32m'
    red='\e[1;31m'
fi
status()  { echo -e "\n${cyan}[MSYS2 CI]${normal} ${@}\n"; }
success() { echo -e "\n${green}[MSYS2 CI] SUCCESS:${normal} ${@}.\n"; exit 0; }
failure() { echo -e "\n${red}[MSYS2 CI] FAILURE:${normal} ${@}.\n"; exit 1; }
gitconf() { test -n "$(git config ${1})" && return 0; git config --global "${1}" "${2}"; }
execute() { status "${package:+$package: }${1}"; ${@:2} || failure "${package:+$package: }${1} failed"; }

# Detect
cd "$(dirname "$0")"
commit_range="$(git log -1 --pretty=format:%P | cut -d' ' -f1)..HEAD"
files=($(git show --pretty=format: --name-only ${commit_range} | sort -u))
for file in "${files[@]}"; do [[ "${file}" = */PKGBUILD ]] && packages+=("${file%/PKGBUILD}"); done
test -n "${files}"    || failure 'Could not detect changed files'
test -z "${packages}" && success 'No changes in package recipes'

# Prepare
execute 'Upgrading the system' pacman --sync --refresh --refresh --sysupgrade --noconfirm --noprogressbar
gitconf user.name  'MSYS2 Continuous Integration' || failure 'Could not configure Git for makepkg'
gitconf user.email 'ci@msys2.org'                 || failure 'Could not configure Git for makepkg'

# Build and install
status 'Building changed packages:'
for package in "${packages[@]}"; do printf "\t${package}\n"; done
for package in "${packages[@]}"; do
    cd "${package}"
    execute     'Installing dependencies' makepkg-mingw --skippgpcheck --noconfirm --noprogressbar --syncdeps --noprepare --nobuild
    execute     'Building'                makepkg-mingw --skippgpcheck --noextract --nocheck
    yes|execute 'Installing'              pacman --upgrade *.pkg.tar.xz
    cd - > /dev/null
done
success 'All packages built and installed successfully'
