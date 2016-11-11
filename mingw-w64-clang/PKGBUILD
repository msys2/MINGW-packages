# Maintainer: Martell Malone <martellmalone@gmail.com>
# Maintainer: Alexey Pavlov <alexpux@gmail.com>
# Contributor: Ray Donnelly <mingw.android@gmail.com>
# Contributor: Mateusz Mikuła <mati865@gmail.com>

# choose the compiler that will be used to build clang
# clang cannot build clang 64 bit due to mingw-w64 headers bugs; 32 bit clang is fine
_compiler=gcc   # clang, gcc

_realname=clang
pkgbase=mingw-w64-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}"
         "${MINGW_PACKAGE_PREFIX}-clang-analyzer"
         "${MINGW_PACKAGE_PREFIX}-clang-tools-extra"
         "${MINGW_PACKAGE_PREFIX}-compiler-rt"
         "${MINGW_PACKAGE_PREFIX}-libc++abi"
         "${MINGW_PACKAGE_PREFIX}-libc++"
         "${MINGW_PACKAGE_PREFIX}-lld"
         "${MINGW_PACKAGE_PREFIX}-libunwind"
         "${MINGW_PACKAGE_PREFIX}-llvm"
         "${MINGW_PACKAGE_PREFIX}-lldb"
         )
pkgver=3.9.0
pkgrel=2
pkgdesc="C language family frontend for LLVM (mingw-w64)"
arch=('any')
url="http://llvm.org"
license=("custom:University of Illinois/NCSA Open Source License")
makedepends=("${MINGW_PACKAGE_PREFIX}-cmake"
             $([[ "$_compiler" == "gcc" ]] && echo \
               "${MINGW_PACKAGE_PREFIX}-gcc")
             $([[ "$_compiler" == "clang" ]] && echo \
               "${MINGW_PACKAGE_PREFIX}-clang")
             "${MINGW_PACKAGE_PREFIX}-libffi"
             "${MINGW_PACKAGE_PREFIX}-pkg-config"
             "${MINGW_PACKAGE_PREFIX}-python3-sphinx"
             "${MINGW_PACKAGE_PREFIX}-python2"
             "tar"
             "groff")
depends=("${MINGW_PACKAGE_PREFIX}-gcc")
#options=('debug' '!strip')
source=(http://llvm.org/releases/${pkgver}/llvm-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/cfe-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/compiler-rt-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/test-suite-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/libcxx-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/libcxxabi-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/clang-tools-extra-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/lld-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/lldb-${pkgver}.src.tar.xz{,.sig}
        http://llvm.org/releases/${pkgver}/libunwind-${pkgver}.src.tar.xz{,.sig}
        "0001-genlib-named-as-llvm-dlltool.patch"
        "0002-COFF-Fix-short-import-lib-import-name-type-bitshift.patch"
        "0003-mingw-w64-use-MSVC-style-ByteAlignment.patch"
        "0004-killthedoctor-mingw.patch"
        "0005-Fix-GetHostTriple-for-mingw-w64-in-msys.patch"
        "0006-use-DESTDIR-on-windows.patch"
        "0101-Revert-Revert-r253898-and-r253899-this-breaks-mingw-.patch"
        "0102-mingw-w64-setup-new-defaults-for-target.patch"
        "0103-mingw-w64-enable-support-for-__declspec-selectany.patch"
        "0104-mingw-w64-support-static-builds-of-libc.patch"
        "0105-mingw-enable-static-libclang.patch"
        "0106-generate-libclang-instead-of-liblibclang.patch"
        "0107-dont-create-cl-mingw.patch"
        "0108-experimental-workaround-for-limits-bug.patch"
        "0109-Set-the-x86-arch-name-to-i686-for-mingw-w64.patch"
        "0110-enable-support-for-float128.patch"
        "0111-fix-clang-with-libstdc.patch"
        "0201-mingw-w64-__udivdi3-mangle-hack.patch"
        "0202-mingw-fixes-for-compiler-rt.patch"
        "0301-COFF-gnu-driver-support.patch"
        "0401-mingw-w64-hack-and-slash-fixes-for-libc.patch"
        "0402-fix-compilation-with-gcc.patch"
        "0501-lldb-mingw-fixes.patch"
        "0502-hack-to-use-64-bit-time-for-mingw.patch"
        "0601-libunwind-add-support-for-mingw-w64.patch")
# Some patch notes :)
#0001-0099 -> llvm
#0101-0199 -> clang
#0201-0299 -> rt
#0301-0399 -> lld
#0401-0499 -> c++
#0501-0599 -> lldb
#0601-0699 -> libunwind
sha256sums=('66c73179da42cee1386371641241f79ded250e117a79f571bbd69e56daa48948'
            'SKIP'
            '7596a7c7d9376d0c89e60028fe1ceb4d3e535e8ea8b89e0eb094e0dcb3183d28'
            'SKIP'
            'e0e5224fcd5740b61e416c549dd3dcda92f10c524216c1edb5e979e42078a59a'
            'SKIP'
            'fdc261b686a75351e691be36a4cbad141096998c03d1487c35b6253d8e5bf408'
            'SKIP'
            'd0b38d51365c6322f5666a2a8105785f2e114430858de4c25a86b49f227f5b06'
            'SKIP'
            'b037a92717856882e05df57221e087d7d595a2ae9f170f7bc1a23ec7a92c8019'
            'SKIP'
            '5b7aec46ec8e999ec683c87ad744082e1133781ee4b01905b4bdae5d20785f14'
            'SKIP'
            '986e8150ec5f457469a20666628bf634a5ca992a53e157f3b69dbc35056b32d9'
            'SKIP'
            '61280e07411e3f2b4cca0067412b39c16b0a9edd19d304d3fc90249899d12384'
            'SKIP'
            '66675ddec5ba0d36689757da6008cb2596ee1a9067f4f598d89ce5a3b43f4c2b'
            'SKIP'
            'a69d60c2ae36f253f34ab3cfaaa4ff10522692dd4a1a646f2f8fa8996fbd026b'
            '89e86d7f53b97bbaef6ee02aa817e979bc122b4844e237b2f5f2af8c268c44c4'
            '3c429a6762c66ffd18b0f378debe0527e15f27e5caa0bb47d2e88f8afe68b093'
            'e83fe9effaa3d0ba21d3bb98bfd471dc0acd2dd99c72695fb33685d69f8e2a76'
            '0804146b32138d55c611336cc81e1783c29a8fab0fe62f248ba1ad7acc711c4d'
            '76bcdcae0ef3a4d3ae7082b7fcd668e9560e63fb82068c3f889f9e89b9becf4a'
            '31e0f242f4463cadc1b867a87b38e4c2f689e70fdd6d64a44dcc3784f352b20f'
            'ba703d3d0f100d02ba01501319e6ec29565a199176fb20d11f89fa31b479df5f'
            'b00b3e2395d9262c999c6865da59837f0712454803e0d4e776181267df89f083'
            '0c570da0d1357cfef276da685b67118d48a6a6f5a0fc4e281c2925c10f8be9ca'
            '0e45e76ee6d6658de52edb7b508a8bcc9f10ff0b295ff2a4e35577136a40c6a5'
            'ddc914887e512e0767b1465f17c80378cf38a041795538b37a40ac57889164fb'
            'b03cfc7ebbbfb847e88ae3569d9dcafb01f179b06f1312de29fbd5b7cf906617'
            'd6c483a99973ae674e85f8d816cb34e1538ee4d9cc64bfe63735322d86503f7c'
            '13a95a61e9c1c44c18a69947734e07515332a549446394f48b86b52511d221de'
            '7abd9894abe9d28da975fadbad27478c308ed1455a5130091ad0ffd0351bfa52'
            '72cae8e17236d6ad625641a6661ce179b8446bd5119f32a352e5b3b9c1a3f397'
            'fb1ef06b26e88d37d52c3e0b3b261089e92bb7c08231ec8fa234465fdbdab308'
            'c19a3e49f692eba9143bb67c39a9e6df33fa604d85b0b7834d99cdd58a28d23a'
            '852d55907b469739fca96b043e41c596824ad9d933268ce65a82100b975e91fb'
            'd9b46363c0db63316bdaa29580c446bfe5bc7b43eb8d00f894b72415066da53e'
            '461c75905768ce9ed14db48e6c959695f8a61c58c60486c2671b6d4d10923bad'
            '0371289d17563a2f29dd7041349e7fe24ed5f217c548c8bff93b47c5b5df2d20'
            '4be9205a90b3ed2d23b8e84cac36697c5a85026b6ab3193061fbb0b3915d76c9'
            '0a5529a5e9871ec5252c3853f1fdda69253c1a7505837e9c45ae14bcb76a8660')
validpgpkeys=('B6C8F98282B944E3B0D5C2530FC3042E345AD05D') # Hans Wennborg, Google.
noextract=(cfe-${pkgver}.src.tar.xz
           libcxx-${pkgver}.src.tar.xz
           lldb-${pkgver}.src.tar.xz
           test-suite-${pkgver}.src.tar.xz)

prepare() {
  plain "Extracting clang-${pkgver}.src.tar.xz due to symlink(s) without pre-existing target(s)"
  [[ -d ${srcdir}/cfe-${pkgver} ]] && rm -rf ${srcdir}/cfe-${pkgver}
  [[ -d ${srcdir}/cfe-${pkgver} ]] || tar -xJvf ${srcdir}/cfe-${pkgver}.src.tar.xz -C ${srcdir} || true

  plain "Extracting libcxx-${pkgver}.src.tar.xz due to symlink(s) without pre-existing target(s)"
  [[ -d ${srcdir}/libcxx-${pkgver} ]] && rm -rf ${srcdir}/libcxx-${pkgver}
  [[ -d ${srcdir}/libcxx-${pkgver} ]] || tar -xJvf ${srcdir}/libcxx-${pkgver}.src.tar.xz -C ${srcdir} || true

  plain "Extracting lldb-${pkgver}.src.tar.xz due to symlink(s) without pre-existing target(s)"
  [[ -d ${srcdir}/lldb-${pkgver} ]] && rm -rf ${srcdir}/lldb-${pkgver}
  [[ -d ${srcdir}/lldb-${pkgver} ]] || tar -xJvf ${srcdir}/lldb-${pkgver}.src.tar.xz -C ${srcdir} || true

  cd "${srcdir}/llvm-${pkgver}.src"
  rm -rf lib/DLLDriver/CMakeLists.txt \
    lib/DLLDriver/Config.h \
    lib/DLLDriver/DllDriver.cpp \
    lib/DLLDriver/LLVMBuild.txt \
    lib/DLLDriver/Librarian.cpp \
    lib/DLLDriver/ModuleDef.cpp \
    lib/DLLDriver/Options.td \
    include/llvm/DllDriver/DllDriver.h || true
  patch -p1 -i "${srcdir}/0001-genlib-named-as-llvm-dlltool.patch"
  patch -p1 -i "${srcdir}/0002-COFF-Fix-short-import-lib-import-name-type-bitshift.patch"
  patch -p1 -i "${srcdir}/0003-mingw-w64-use-MSVC-style-ByteAlignment.patch"
  patch -p1 -i "${srcdir}/0004-killthedoctor-mingw.patch"
  patch -p1 -i "${srcdir}/0005-Fix-GetHostTriple-for-mingw-w64-in-msys.patch"
  patch -p1 -i "${srcdir}/0006-use-DESTDIR-on-windows.patch"

  cd "${srcdir}/cfe-${pkgver}.src"
  patch -p1 -i "${srcdir}/0101-Revert-Revert-r253898-and-r253899-this-breaks-mingw-.patch"
  #patch -p1 -i "${srcdir}/0012-mingw-w64-setup-new-defaults-for-target.patch"
  patch -p1 -i "${srcdir}/0103-mingw-w64-enable-support-for-__declspec-selectany.patch"
  patch -p1 -i "${srcdir}/0104-mingw-w64-support-static-builds-of-libc.patch"
  patch -p1 -i "${srcdir}/0105-mingw-enable-static-libclang.patch"
  patch -p1 -i "${srcdir}/0106-generate-libclang-instead-of-liblibclang.patch"
  patch -p1 -i "${srcdir}/0107-dont-create-cl-mingw.patch"
  patch -p1 -i "${srcdir}/0108-experimental-workaround-for-limits-bug.patch"
  patch -p1 -i "${srcdir}/0109-Set-the-x86-arch-name-to-i686-for-mingw-w64.patch"
  patch -p1 -R -i "${srcdir}/0110-enable-support-for-float128.patch"
  patch -p1 -i "${srcdir}/0111-fix-clang-with-libstdc.patch"

  cd "${srcdir}/compiler-rt-${pkgver}.src"
  # patch -p1 -i "${srcdir}/0201-mingw-w64-__udivdi3-mangle-hack.patch"
  patch -p1 -i "${srcdir}/0202-mingw-fixes-for-compiler-rt.patch"

  cd "${srcdir}/lld-${pkgver}.src"
  patch -p1 -i "${srcdir}/0301-COFF-gnu-driver-support.patch"

  cd "${srcdir}/libcxx-${pkgver}.src"
  patch -p1 -i "${srcdir}/0401-mingw-w64-hack-and-slash-fixes-for-libc.patch"
  patch -p1 -i "${srcdir}/0402-fix-compilation-with-gcc.patch"

  cd "${srcdir}/lldb-${pkgver}.src"
  patch -p1 -i "${srcdir}/0501-lldb-mingw-fixes.patch"
  # this hack allows to build 32 bit lldb but probably breaks XP support
  patch -p1 -i "${srcdir}/0502-hack-to-use-64-bit-time-for-mingw.patch"

  cd "${srcdir}/libunwind-${pkgver}.src"
  patch -p1 -i "${srcdir}/0601-libunwind-add-support-for-mingw-w64.patch"

  # At the present, clang must reside inside the LLVM source code tree to build
  # See http://llvm.org/bugs/show_bug.cgi?id=4840

  cd "${srcdir}"/llvm-${pkgver}.src
  # note you need to ignore failure if there is stuff that already exists
  mv "${srcdir}/cfe-${pkgver}.src"               tools/clang | true
  mv "${srcdir}/clang-tools-extra-${pkgver}.src" tools/clang/tools/extra | true
  mv "${srcdir}/lld-${pkgver}.src"               tools/lld | true
  mv "${srcdir}/lldb-${pkgver}.src"              tools/lldb | true
  mv "${srcdir}/compiler-rt-${pkgver}.src"       projects/compiler-rt | true
  mv "${srcdir}/libcxxabi-${pkgver}.src"         projects/libcxxabi | true
  mv "${srcdir}/libcxx-${pkgver}.src"            projects/libcxx | true
  mv "${srcdir}/libunwind-${pkgver}.src"         projects/libunwind | true
  #mv "${srcdir}/testsuite-${pkgver}.src"         projects/test-suite | true
}

build() {
  cd "${srcdir}"

  [[ -d build-${CARCH} ]] && rm -rf build-${CARCH}
  mkdir build-${CARCH} && cd build-${CARCH}

  local -a extra_config

  if check_option "debug" "y"; then
    extra_config+=(-DCMAKE_BUILD_TYPE=Debug)
    VERBOSE="VERBOSE=1"
  else
    extra_config+=(-DCMAKE_BUILD_TYPE=Release)
  fi

  # Include location of libffi headers in CPPFLAGS
  FFI_INCLUDE_DIR="$(pkg-config --cflags libffi)"
  FFI_INCLUDE_DIR=$(echo $FFI_INCLUDE_DIR | sed 's|-I||g')

  # "Ninja" cant install each component seperately
  # https://github.com/martine/ninja/issues/932

  if [ "${_compiler}" == "gcc" ]; then
    export CC='gcc'
    export CXX='g++'
  elif [ "${_compiler}" == "clang" ]; then
    #export CC='clang -stdlib=libc++'
    #export CXX='clang++ -stdlib=libc++'
    export CC='clang'
    export CXX='clang++'
  else
    msg "undefined compiler"
    exit 1
  fi

  MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=" \
  ${MINGW_PREFIX}/bin/cmake.exe \
    -G"MSYS Makefiles" \
    -DCMAKE_SYSTEM_IGNORE_PATH=/usr/lib \
    -DCMAKE_MAKE_PROGRAM="/usr/bin/make.exe" \
    -DFFI_INCLUDE_DIR="${FFI_INCLUDE_DIR}" \
    -DCMAKE_C_FLAGS="${CFLAGS}" \
    -DCMAKE_CXX_FLAGS="${CXXFLAGS} ${CPPFLAGS}" \
    -DCMAKE_INSTALL_PREFIX=${MINGW_PREFIX} \
    -DLLVM_TARGETS_TO_BUILD="ARM;X86" \
    -DLLVM_ENABLE_ASSERTIONS=OFF \
    -DLLVM_ENABLE_THREADS=1 \
    -DPYTHON_EXECUTABLE=${MINGW_PREFIX}/bin/python2.exe \
    -DLLVM_ENABLE_FFI=ON \
    -DLLVM_ENABLE_SPHINX=ON \
    -DCMAKE_CXX_FLAGS="-D_GNU_SOURCE" \
    -DLIBCLANG_BUILD_STATIC=ON \
    -DLIBCXX_ENABLE_SHARED=OFF \
    -DLIBCXXABI_ENABLE_SHARED=OFF \
    -DLIBUNWIND_ENABLE_SHARED=OFF \
    -DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=OFF \
    "${extra_config[@]}" \
    ../llvm-${pkgver}.src

  make ${VERBOSE}

  # Disable automatic installation of components that go into subpackages
  # -i.orig to check what has been removed in-case it starts dropping more than it should
  #
  sed -i.orig '/\(clang\|lld\|lldb\)\/cmake_install.cmake/d' tools/cmake_install.cmake
  sed -i.orig '/\(extra\|scan-build\|scan-view\)\/cmake_install.cmake/d' tools/clang/tools/cmake_install.cmake
# sed -i.orig '/\(compiler-rt\|libcxxabi\|libcxx\)\/cmake_install.cmake/d' projects/cmake_install.cmake
  sed -i.orig '/\(compiler-rt\|libcxxabi\|libcxx\|libunwind\)\/cmake_install.cmake/d' projects/cmake_install.cmake
}

#check() {
#  cd "${srcdir}"/build-${CARCH}
#  # Remove || true once testcase doesn't cause failures.
#  make check || true
#}

package_llvm() {
  pkgdesc="Low Level Virtual Machine (mingw-w64)"
  depends=("${MINGW_PACKAGE_PREFIX}-gcc-libs") # "compiler-rt"
  #install=llvm-${CARCH}.install

  cd "${srcdir}"/llvm-${pkgver}.src

  make -C ../build-${CARCH} DESTDIR="${pkgdir}" install

  install -Dm644 LICENSE.TXT "${pkgdir}${MINGW_PREFIX}/share/licenses/llvm/LICENSE"

  # Install CMake stuff
  install -d "${pkgdir}"${MINGW_PREFIX}/share/llvm/cmake/{modules,platforms}
  install -Dm644 "${srcdir}"/llvm-${pkgver}.src/cmake/modules/*.cmake "${pkgdir}"${MINGW_PREFIX}/share/llvm/cmake/modules/
  install -Dm644 "${srcdir}"/llvm-${pkgver}.src/cmake/platforms/*.cmake "${pkgdir}"${MINGW_PREFIX}/share/llvm/cmake/platforms/

  # FileCheck is needed to build rust.
  install -Dm755 "${srcdir}"/build-${CARCH}/bin/FileCheck.exe "${pkgdir}${MINGW_PREFIX}/bin/FileCheck.exe"
}

package_compiler-rt() {
  pkgdesc="Runtime libraries for Clang and LLVM (mingw-w64)"
  url="http://compiler-rt.llvm.org/"
  depends=("${MINGW_PACKAGE_PREFIX}-llvm=${pkgver}-${pkgrel}")

  cd "${srcdir}"/llvm-${pkgver}.src
  make -C ../build-${CARCH}/projects/compiler-rt DESTDIR="${pkgdir}" install
 }

package_libcxxabi() {
  pkgdesc="C++ Standard Library Support (mingw-w64)"
  url="http://libcxxabi.llvm.org/"
  depends="${MINGW_PACKAGE_PREFIX}-libunwind"

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/projects/libcxxabi -j1 DESTDIR="${pkgdir}" install
}

package_libcxx() {
  pkgdesc="C++ Standard Library (mingw-w64)"
  url="http://libcxx.llvm.org/"

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/projects/libcxx -j1 DESTDIR="${pkgdir}" install
}

package_libunwind() {
  pkgdesc='A new implementation of a stack unwinder for C++ exceptions (mingw-w64)'
  url='http://llvm.org'

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/projects/libunwind -j1 DESTDIR="${pkgdir}" install
}

package_lldb() {
  pkgdesc="Next generation, high-performance debugger (mingw-w64)"
  url="http://lldb.llvm.org/"
  depends=("${MINGW_PACKAGE_PREFIX}-readline"
           "${MINGW_PACKAGE_PREFIX}-libxml2"
           "${MINGW_PACKAGE_PREFIX}-python2")

  cd "${srcdir}/llvm-${pkgver}.src"

  make -C ../build-${CARCH}/tools/lldb DESTDIR="${pkgdir}" install

  # Compile Python scripts
  python2 -m compileall "${pkgdir}${MINGW_PREFIX}/lib/python2.7/site-packages/lldb"
  python2 -O -m compileall "${pkgdir}${MINGW_PREFIX}/lib/python2.7/site-packages/lldb"

  install -Dm644 tools/lldb/LICENSE.TXT "${pkgdir}${MINGW_PREFIX}/share/licenses/$pkgname/LICENSE"
}

package_lld() {
  pkgdesc="Linker tools for LLVM (mingw-w64)"
  url="http://lld.llvm.org/"

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/tools/lld -j1 DESTDIR="${pkgdir}" install
}

package_clang() {
  pkgdesc="C language family frontend for LLVM (mingw-w64)"
  url="http://clang.llvm.org/"
  depends=("${MINGW_PACKAGE_PREFIX}-llvm=${pkgver}-${pkgrel}")

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/tools/clang DESTDIR="${pkgdir}" install

  # Install static clang library ..
  cp ../build-${CARCH}/lib/libclang.a ${pkgdir}${MINGW_PREFIX}/lib/
}

package_clang-analyzer() {
  pkgdesc="A source code analysis framework (mingw-w64)"
  url="http://clang-analyzer.llvm.org/"
  depends=("${MINGW_PACKAGE_PREFIX}-clang=${pkgver}-${pkgrel}"
           "${MINGW_PACKAGE_PREFIX}-python2")

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/tools/clang/tools/scan-build -j1 DESTDIR="${pkgdir}" install
  make -C ../build-${CARCH}/tools/clang/tools/scan-view -j1 DESTDIR="${pkgdir}" install

  # Use Python 2
  sed -i \
    -e 's|env python$|&2|' \
    -e 's|/usr/bin/python$|&2|' \
    "${pkgdir}"${MINGW_PREFIX}/bin/scan-view

  # Compile Python scripts
  python2 -m compileall "${pkgdir}"${MINGW_PREFIX}/bin/clang-analyzer
  python2 -O -m compileall "${pkgdir}"${MINGW_PREFIX}/lib/clang-analyzer
  install -Dm644 LICENSE.TXT "${pkgdir}"${MINGW_PREFIX}/share/licenses/clang-analyzer/LICENSE
}

package_clang-tools-extra() {
  pkgdesc="Extra tools built using Clang's tooling APIs (mingw-w64)"
  url="http://clang.llvm.org/"

  cd "${srcdir}/llvm-${pkgver}.src"
  make -C ../build-${CARCH}/tools/clang/tools/extra -j1 DESTDIR="${pkgdir}" install
}

# Wrappers
package_mingw-w64-i686-clang(){
  package_clang
}

package_mingw-w64-i686-clang-analyzer(){
  package_clang-analyzer
}

package_mingw-w64-i686-clang-tools-extra(){
  package_clang-tools-extra
}

package_mingw-w64-i686-compiler-rt(){
  package_compiler-rt
}

package_mingw-w64-i686-libc++abi(){
  package_libcxxabi
}

package_mingw-w64-i686-libc++(){
  package_libcxx
}

package_mingw-w64-i686-lld(){
  package_lld
}

package_mingw-w64-i686-lldb(){
  package_lldb
}

package_mingw-w64-i686-libunwind(){
  package_libunwind
}

package_mingw-w64-i686-llvm(){
  package_llvm
}

package_mingw-w64-x86_64-clang(){
  package_clang
}

package_mingw-w64-x86_64-clang-analyzer(){
  package_clang-analyzer
}

package_mingw-w64-x86_64-clang-tools-extra(){
  package_clang-tools-extra
}

package_mingw-w64-x86_64-compiler-rt(){
  package_compiler-rt
}

package_mingw-w64-x86_64-libc++abi(){
  package_libcxxabi
}

package_mingw-w64-x86_64-libc++(){
  package_libcxx
}

package_mingw-w64-x86_64-lld(){
  package_lld
}

package_mingw-w64-x86_64-lldb(){
  package_lldb
}

package_mingw-w64-x86_64-libunwind(){
  package_libunwind
}

package_mingw-w64-x86_64-llvm(){
  package_llvm
}

# vim:set ts=2 sw=2 et:
