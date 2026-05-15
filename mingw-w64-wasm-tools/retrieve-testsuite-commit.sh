#!/usr/bin/sh

TAG="$1"

if [ -z "$1" ]
  then
    echo "Usage: $0 <pkgver>"
    exit 1
fi

CONTENTS=$(curl -s "https://api.github.com/repos/bytecodealliance/wasm-tools/contents/tests/testsuite?ref=v$TAG")

# Ensure that retrieved object is indeed a submodule
CONTENT_TYPE=$(echo "$CONTENTS" | jq -r '.type')
if [ "$CONTENT_TYPE" != 'submodule' ]; then
    echo "Path is not a submodule"
    exit 1
fi

# Get the SHA of the submodule
SUBMODULE_SHA=$(echo "$CONTENTS" | jq -r '.sha')
echo "$SUBMODULE_SHA"
