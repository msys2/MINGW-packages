#!/usr/bin/env bash
#
# Get compatible versions of binaryen and llvm
#
# How to use:
# Change pkgver in PKGBUILD to desired version and then run ./get-compatible-versions.sh.
# This will fetch, print and substitute into the PKGBUILD the compatible binaryen and llvm versions.

pkgver=$(makepkg --printsrcinfo | sed -rn 's/.*pkgver = (.*)/\1/gp')
tag_hash=$(curl -Ls https://github.com/emscripten-core/emsdk/raw/main/emscripten-releases-tags.json | jq ".releases[\"$pkgver\"]" | sed s/\"//g)
deps_file=$(curl -Ls "https://chromium.googlesource.com/emscripten-releases/+/$tag_hash/DEPS?format=TEXT" | base64 -d)
binaryen_revision=$(echo "$deps_file" | sed -rn "s/.*'binaryen_revision': '(.*)',/\\1/gp")
llvm_project_revision=$(echo "$deps_file" | sed -rn "s/.*'llvm_project_revision': '(.*)',/\\1/gp")
sed -i "s/_binaryen_revision=.*/_binaryen_revision=$binaryen_revision/g" PKGBUILD
sed -i "s/_llvm_project_revision=.*/_llvm_project_revision=$llvm_project_revision/g" PKGBUILD
