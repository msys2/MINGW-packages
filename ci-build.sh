#!/bin/bash

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

# Functions
failure()       { colored; printf "\n${red}[MSYS2 CI] FAILURE:${normal} ${1}.\n\n"; exit 1; }
success()       { colored; printf "\n${green}[MSYS2 CI] SUCCESS:${normal} ${1}.\n\n"; exit 0; }
status()        { colored; printf "\n${cyan}[MSYS2 CI]${normal} ${1}\n\n"; printf "${2:+\t%s\n}" "${2:+${@:2}}" | sed 's/.*::/\t/'; }
colored()       { [[ -t 1  && -z "${normal}" ]] || return; normal='\e[0m'; red='\e[1;31m'; green='\e[1;32m'; cyan='\e[1;36m'; }
execute()       { cd "${package:-.}"; status "${package:+$package: }${1}"; ${@:2} || failure "${package:+$package: }${1} failed"; cd - > /dev/null; }
git_config()    { test -n "$(git config ${1})" && return 0; git config --global "${1}" "${2}" || failure 'Could not configure Git for makepkg'; }
as_list()       { result=(); local lines=(); while IFS= read -r line; do lines+=("${line}"); [[ "${line}" = ${1} ]] && result+=("${line}"); done <<<"${2}"; test -n "${lines}"; }
list_changes()  { as_list "${1}" "$(git log "${@:2}" upstream/master.. | grep -v ^$ | sort -u)" || as_list "${1}" "$(git log "${@:2}" HEAD^.. | grep -v ^$ | sort -u)"; }
list_packages() { packages=(); list_changes '*/PKGBUILD' --pretty=format: --name-only && for recipe in "${result[@]}"; do packages+=("${recipe%/PKGBUILD}"); done; }
list_commits()  { commits=();  list_changes '*' --pretty=format:'%ai::[%h] %s' && commits=("${result[@]}"); }

# Configure
cd "$(dirname "$0")"
git_config user.email 'ci@msys2.org'
git_config user.name  'MSYS2 Continuous Integration'
git remote add upstream 'https://github.com/Alexpux/MINGW-packages'
git fetch --quiet upstream

# Detect
list_commits  || failure 'Could not detect added commits'
list_packages || failure 'Could not detect changed files'
status 'Processing changes' "${commits[@]}"
test -z "${packages}" && success 'No changes in package recipes'

# Build
status 'Building packages' "${packages[@]}"
execute 'Upgrading the system' pacman --noconfirm --noprogressbar --sync --refresh --refresh --sysupgrade
for package in "${packages[@]}"; do
    execute 'Building' makepkg-mingw --noconfirm --noprogressbar --skippgpcheck --nocheck --syncdeps --rmdeps --cleanbuild
    yes|execute 'Installing' pacman --noprogressbar --upgrade *.pkg.tar.xz
done
success 'All packages built and installed successfully'
