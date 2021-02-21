#!/bin/bash
set -e

. PKGBUILD
_keyserver=(
    "keyserver.ubuntu.com"
    "keys.gnupg.net"
    "pgp.mit.edu"
    "keys.openpgp.org"
)
for key in "${validpgpkeys[@]}"; do
    for server in "${_keyserver[@]}"; do
        timeout 20 /usr/bin/gpg --keyserver "${server}" --recv "${key}" && break || true
    done
done
