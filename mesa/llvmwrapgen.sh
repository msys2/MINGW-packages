#!/bin/sh
# Get LLVM libraries
if [ ${nollvmconfig} = 1 ] && [ ${llvm_static} = true ]; then
  llvmlibs='libLLVMCoroutines.a libLLVMipo.a libLLVMInstrumentation.a libLLVMVectorize.a libLLVMLinker.a libLLVMIRReader.a libLLVMAsmParser.a libLLVMFrontendOpenMP.a libLLVMX86Disassembler.a libLLVMX86AsmParser.a libLLVMX86CodeGen.a libLLVMCFGuard.a libLLVMGlobalISel.a libLLVMSelectionDAG.a libLLVMAsmPrinter.a libLLVMDebugInfoDWARF.a libLLVMCodeGen.a libLLVMScalarOpts.a libLLVMInstCombine.a libLLVMAggressiveInstCombine.a libLLVMTransformUtils.a libLLVMBitWriter.a libLLVMX86Desc.a libLLVMMCDisassembler.a libLLVMX86Info.a libLLVMMCJIT.a libLLVMExecutionEngine.a libLLVMTarget.a libLLVMAnalysis.a libLLVMProfileData.a libLLVMRuntimeDyld.a libLLVMObject.a libLLVMTextAPI.a libLLVMMCParser.a libLLVMBitReader.a libLLVMMC.a libLLVMDebugInfoCodeView.a libLLVMDebugInfoMSF.a libLLVMCore.a libLLVMRemarks.a libLLVMBitstreamReader.a libLLVMBinaryFormat.a libLLVMSupport.a libLLVMDemangle.a'
fi

if ! [ ${nollvmconfig} = 1 ] && [ ${llvm_static} = true ]; then
  llvmlibs=$(${MINGW_PREFIX}/bin/llvm-config --link-static --libnames engine coroutines 2>&1)
fi

if [ ${llvm_static} = false ]; then
  llvmlibs=libLLVM
fi

# Get LLVM RTTI status
rtti=false
if [ $(${MINGW_PREFIX}/bin/llvm-config --has-rtti 2>&1) = YES ]; then
  rtti=true
fi

# Convert llvm-config libraries list into a Python list
llvmlibs="${llvmlibs//.a/}"
llvmlibs=\'"${llvmlibs// /\', \'}"\'

# Get libraries directory
if [ ${llvm_static} = false ]; then
  wraplibdir=bin
fi
if [ ${llvm_static} = true ]; then
  wraplibdir=lib
fi

# Generate a Meson wrap file for LLVM
mkdir -p ./subprojects/llvm
FILE="./subprojects/llvm/meson.build"
/bin/cat <<EOM >${FILE}
project('llvm', ['cpp'])

cpp = meson.get_compiler('cpp')

_deps = []
_search = '$(cygpath -m ${MINGW_PREFIX})/${wraplibdir}'
foreach d : [${llvmlibs}]
  _deps += cpp.find_library(d, dirs : _search, static : ${llvm_static})
endforeach

dep_llvm = declare_dependency(
  include_directories : include_directories('$(cygpath -m ${MINGW_PREFIX})/include'),
  dependencies : _deps,
  version : '$(/usr/bin/pacman -Q ${MINGW_PACKAGE_PREFIX}-llvm | cut -d" " -f 2 | cut -d"-" -f 1)',
)

has_rtti = ${rtti}
irbuilder_h = files('$(cygpath -m ${MINGW_PREFIX})/include/llvm/IR/IRBuilder.h')
EOM