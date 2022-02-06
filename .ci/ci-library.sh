#!/bin/bash

# Continuous Integration Library for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

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

# Git configuration
git_config() {
    local name="${1}"
    local value="${2}"
    test -n "$(git config ${name})" && return 0
    git config --global "${name}" "${value}" && return 0
    failure 'Could not configure Git for makepkg'
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

# Recipe quality
check_recipe_quality() {
    # TODO: remove this option when not anymore needed
    if test -n "${DISABLE_QUALITY_CHECK}"; then
        echo 'This feature is disabled.'
        return 0
    fi
    saneman --format='\t%l:%c %p:%c %m' --verbose --no-terminal "${packages[@]}"
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
