#!/bin/bash

# AppVeyor and Drone Continuous Integration for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

# Functions
failure()       { colored; printf "\n${red}[MSYS2 CI] FAILURE:${normal} ${1}.\n\n"; exit 1; }
success()       { colored; printf "\n${green}[MSYS2 CI] SUCCESS:${normal} ${1}.\n\n"; exit 0; }
status()        { colored; printf "\n${cyan}[MSYS2 CI]${normal} ${1}\n\n"; printf "${2:+\t%s\n}" "${2:+${@:2}}"; }
commits()       { printf "\n\t${1}\n"; list_changes --pretty=format:"%ai::[%h] %s" | awk -F:: '{print "\t"$2}'; }
colored()       { [[ -t 1  && -z "${normal}" ]] || return; normal='\e[0m'; red='\e[1;31m'; green='\e[1;32m'; cyan='\e[1;36m'; }
execute()       { cd "${package:-.}"; status "${package:+$package: }${1}"; ${@:2} || failure "${package:+$package: }${1} failed"; cd - > /dev/null; }
git_config()    { test -n "$(git config ${1})" && return 0; git config --global "${1}" "${2}" || failure 'Could not configure Git for makepkg'; }
list_changes()  { local since_master; while IFS= read -r line; do test -n "${line}" && since_master+=("${line}"); done <<< "$(git log "${@}" upstream/master.. | sort -u)"
                  local from_head;    while IFS= read -r line; do test -n "${line}" && from_head+=("${line}");    done <<< "$(git log "${@}" HEAD^.. | sort -u)"
                  [[ -n "${since_master[@]}" ]] && printf '%s\n' "${since_master[@]}" || printf '%s\n' "${from_head[@]}"; }

# Configure
cd "$(dirname "$0")"
git_config user.email 'ci@msys2.org'
git_config user.name  'MSYS2 Continuous Integration'
git remote add upstream 'https://github.com/Alexpux/MINGW-packages'
git fetch --quiet upstream

# Detect
files=($(list_changes --pretty=format: --name-only))
for file in "${files[@]}"; do [[ "${file}" = */PKGBUILD ]] && packages+=("${file%/PKGBUILD}"); done
test -n "${files}"    || failure 'Could not detect changed files'
test -z "${packages}" && success 'No changes in package recipes'

# Build
status 'Building packages' "${packages[@]}"
commits 'Changed by commits'
execute 'Upgrading the system' pacman --noconfirm --noprogressbar --sync --refresh --refresh --sysupgrade
for package in "${packages[@]}"; do
    execute 'Building' makepkg-mingw --noconfirm --noprogressbar --skippgpcheck --nocheck --syncdeps --rmdeps --cleanbuild
    yes|execute 'Installing' pacman --noprogressbar --upgrade *.pkg.tar.xz
done
success 'All packages built and installed successfully'
