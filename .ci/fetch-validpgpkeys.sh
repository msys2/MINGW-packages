#!/bin/bash

set -e

# use ./ otherwise it resolves the whole PATH
. ./PKGBUILD

_keyservers=(
    "keyserver.ubuntu.com"
    "pgp.mit.edu"
    "keys.openpgp.org"
)

for key in "${validpgpkeys[@]}"; do
    if /usr/bin/gpg --list-keys "${key}" >/dev/null 2>&1; then
        continue
    fi

    fetched=false
    for server in "${_keyservers[@]}"; do
        if timeout 20 /usr/bin/gpg --keyserver "${server}" --recv-keys "${key}"; then
            fetched=true
            break
        fi
    done

    # warning because makepkg will fail after checking the archive's real PGP key
    if [[ "${fetched}" != true ]]; then
        echo "warning: failed to fetch PGP key ${key}" >&2
    fi
done
