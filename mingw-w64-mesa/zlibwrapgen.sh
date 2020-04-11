#!/bin/sh
# Generate a Meson wrap file for zlib
mkdir -p ./subprojects/zlib
FILE="./subprojects/zlib/meson.build"
/bin/cat <<EOM >${FILE}
project('zlib', ['cpp'])

cpp = meson.get_compiler('cpp')

_deps = []
_search = '$(cygpath -m ${MINGW_PREFIX})/lib'
foreach d : ['libz']
  _deps += cpp.find_library(d, dirs : _search, static : true)
endforeach

zlib_dep = declare_dependency(
  include_directories : include_directories('$(cygpath -m ${MINGW_PREFIX})/include'),
  dependencies : _deps,
  version : '$(/usr/bin/pacman -Q ${MINGW_PACKAGE_PREFIX}-zlib | cut -d" " -f 2 | cut -d"-" -f 1)',
)
EOM