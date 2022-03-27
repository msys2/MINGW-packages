#!/bin/bash

# https://www.apache.org/dyn/closer.lua?action=download&filename=arrow/KEYS
# has valid keys for Apache Arrow sources.

set -e

keyring=./apache-arrow-keys.gpg
rm -f ${keyring}
curl \
  --fail \
  --location \
  --no-progress-meter \
  'https://www.apache.org/dyn/closer.lua?action=download&filename=arrow/KEYS' | \
  gpg \
    --no-default-keyring \
    --keyring ${keyring} \
    --import | \
    : # Ignore import error
i=0
gpg \
  --no-default-keyring \
  --keyring ${keyring} \
  --list-public-keys \
  --with-colons | \
  grep -A1 '^pub:' | \
  grep '^fpr:' | \
  cut -d: -f10 | \
  while read fingerprint; do
    if [ $i -eq 0 ]; then
      echo -n "validpgpkeys=('$fingerprint'"
    else
      echo
      echo -n "              '$fingerprint'"
    fi
    i=$((i+1))
  done
echo ")"
