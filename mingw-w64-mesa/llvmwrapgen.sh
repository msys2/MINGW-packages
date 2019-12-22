#!/bin/sh
# Get LLVM libraries
llvmlibs=$(${MINGW_PREFIX}/bin/llvm-config --libnames engine coroutines)

# Get LLVM RTTI status
rtti=false
if [ $(${MINGW_PREFIX}/bin/llvm-config --has-rtti) = YES ]; then
  rtti=true
fi

# Convert llvm-config output into a Python list
llvmlibs="${llvmlibs//.a/}"
llvmlibs=\'"${llvmlibs// /\', \'}"\'

# Generate a Meson wrap file for LLVM
mkdir -p ./subprojects/llvm
FILE="./subprojects/llvm/meson.build"
/bin/cat <<EOM >${FILE}
project('llvm', ['cpp'])

cpp = meson.get_compiler('cpp')

_deps = []
_search = '$(cygpath -m ${MINGW_PREFIX})/lib'
foreach d : [${llvmlibs}]
  _deps += cpp.find_library(d, dirs : _search)
endforeach

dep_llvm = declare_dependency(
  include_directories : include_directories('$(cygpath -m ${MINGW_PREFIX})/include'),
  dependencies : _deps,
  version : '$(${MINGW_PREFIX}/bin/llvm-config --version)',
)

has_rtti = ${rtti}
irbuilder_h = files('$(cygpath -m ${MINGW_PREFIX})/include/llvm/IR/IRBuilder.h')
EOM