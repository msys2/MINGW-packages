#!/bin/bash

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

# Functions
failure()       { colored; printf "\n${red}[MSYS2 CI] FAILURE:${normal} ${1}.\n\n"; exit 1; }
success()       { colored; printf "\n${green}[MSYS2 CI] SUCCESS:${normal} ${1}.\n\n"; exit 0; }
status()        { colored; printf "\n${cyan}[MSYS2 CI]${normal} ${1}\n\n"; printf "${2:+\t%s\n}" "${2:+${@:2}}"; }
colored()       { [[ -t 1 && -z "${normal}" ]] || return; normal='\e[0m'; red='\e[1;31m'; green='\e[1;32m'; cyan='\e[1;36m'; }
execute()       { cd "${package:-.}"; status "${package:+$package: }${1}"; ${@:2} || failure "${package:+$package: }${1} failed"; cd - > /dev/null; }
git_config()    { test -n "$(git config ${1})" && return 0; git config --global "${1}" "${2}" || failure 'Could not configure Git for makepkg'; }
as_list()       { local -n nameref="${1}"; local result=1; while IFS= read -r line; do test -z "${line}" && continue; result=0; [[ "${line}" = ${2} ]] && nameref+=("${line/${3}/}"); done <<<"${4}"; return ${result}; }
list_changes()  { as_list "${@:1:3}" "$(git log "${@:4}" upstream/master.. | sort -u)" || as_list "${@:1:3}" "$(git log "${@:4}" HEAD^.. | sort -u)"; }
list_packages() { list_changes packages '*/PKGBUILD' '%/PKGBUILD' --pretty=format: --name-only; }
list_commits()  { list_changes commits '*' '#*::' --pretty=format:'%ai::[%h] %s'; }

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
