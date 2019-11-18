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

# Get MSYS2 Mingw-w64 runtime location
msysloc=$(${MINGW_PREFIX}/bin/python3 -c "import sys; print(sys.executable)")
msysloc=${msysloc//\"/}
msysloc=${msysloc//\\/\/}
msysloc=${msysloc:0:-16}

# Generate a Meson wrap file for LLVM
mkdir -p ./subprojects/llvm
FILE="./subprojects/llvm/meson.build"
/bin/cat <<EOM >${FILE}
project('llvm', ['cpp'])

cpp = meson.get_compiler('cpp')

_deps = []
_search = '${msysloc}/lib'
foreach d : [${llvmlibs}]
  _deps += cpp.find_library(d, dirs : _search)
endforeach

dep_llvm = declare_dependency(
  include_directories : include_directories('${msysloc}/include'),
  dependencies : _deps,
  version : '$(${MINGW_PREFIX}/bin/llvm-config --version)',
)

has_rtti = ${rtti}
irbuilder_h = files('${msysloc}/include/llvm/IR/IRBuilder.h')
EOM