#!/bin/bash

# Continuous Integration Library for MSYS2
# Author: Renato Silva <br.renatosilva@gmail.com>
# Author: Qian Hong <fracting@gmail.com>

# Enable colors
if [[ -t 1 ]]; then
    normal='\e[0m'
    red='\e[1;31m'
    green='\e[1;32m'
    cyan='\e[1;36m'
fi

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

# Convert lines to array
_as_list() {
    local -n nameref_list="${1}"
    local filter="${2}"
    local strip="${3}"
    local lines="${4}"
    local result=1
    while IFS= read -r line; do
        test -z "${line}" && continue
        result=0
        [[ "${line}" = ${filter} ]] && nameref_list+=("${line/${strip}/}")
    done <<< "${lines}"
    return "${result}"
}

# Changes since master or from head
_list_changes() {
    local list_name="${1}"
    local filter="${2}"
    local strip="${3}"
    local git_options=("${@:4}")
    _as_list "${list_name}" "${filter}" "${strip}" "$(git log "${git_options[@]}" upstream/master.. | sort -u)" ||
    _as_list "${list_name}" "${filter}" "${strip}" "$(git log "${git_options[@]}" HEAD^.. | sort -u)"
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
    local command=("${@:2}")
    cd "${package:-.}"
    message "${status}"
    ${command[@]} || failure "${status} failed"
    cd - > /dev/null
}

# Added commits or changed recipes
list_commits()  { _list_changes commits '*' '#*::' --pretty=format:'%ai::[%h] %s'; }
list_packages() { _list_changes packages '*/PKGBUILD' '%/PKGBUILD' --pretty=format: --name-only; }

# Status functions
failure() { local status="${1}"; local items=("${@:2}"); _status failure "${status}." "${items[@]}"; exit 1; }
success() { local status="${1}"; local items=("${@:2}"); _status success "${status}." "${items[@]}"; exit 0; }
message() { local status="${1}"; local items=("${@:2}"); _status message "${status}"  "${items[@]}"; }
