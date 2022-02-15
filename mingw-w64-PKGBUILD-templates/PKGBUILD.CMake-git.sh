# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-${_realname}
pkgver=1.0.0.r849.gc56db2336
pkgrel=1
pkgdesc="Some package (mingw-w64)"
arch=('any')
mingw_arch=('mingw32' 'mingw64' 'ucrt64' 'clang64' 'clang32')
url="https://github.com/someproject/somepackage"
license=('LICENSE')
makedepends=("${MINGW_PACKAGE_PREFIX}-cmake"
             "${MINGW_PACKAGE_PREFIX}-ninja"
             "${MINGW_PACKAGE_PREFIX}-cc"
             'git')
_commit='ff9ee68d7e31784c6fea3c864a235ad1f32ff026'
source=("${_realname}"::"git+https://github.com/someproject/somepackage.git#commit=${_commit}"
        0001-An-important-fix.patch
        0002-A-less-important-fix.patch)
sha256sums=('SKIP'
            'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb')

pkgver() {
  cd "${srcdir}/${_realname}"

  git describe --long "${_commit}" | sed 's/\([^-]*-g\)/r\1/;s/-/./g;s/^v//g'
}

prepare() {
  cd "${srcdir}/${_realname}"

  patch -Np1 -i "${srcdir}/0001-A-really-important-fix.patch"
  patch -Np1 -i "${srcdir}/0002-A-less-important-fix.patch"
}

build() {
  mkdir -p "${srcdir}/build-${MSYSTEM}" && cd "${srcdir}/build-${MSYSTEM}"

  declare -a extra_config
  if check_option "debug" "n"; then
    extra_config+=("-DCMAKE_BUILD_TYPE=Release")
  else
    extra_config+=("-DCMAKE_BUILD_TYPE=Debug")
  fi

  MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=" \
    "${MINGW_PREFIX}"/bin/cmake.exe \
      -GNinja \
      -DCMAKE_INSTALL_PREFIX="${MINGW_PREFIX}" \
      "${extra_config[@]}" \
      -DBUILD_{SHARED,STATIC}_LIBS=ON \
      ../${_realname}

  "${MINGW_PREFIX}"/bin/cmake.exe --build .
}

check() {
  cd "${srcdir}/build-${MSYSTEM}"

  "${MINGW_PREFIX}"/bin/cmake.exe --build . --target test
}

package() {
  cd "${srcdir}/build-${MSYSTEM}"

  DESTDIR="${pkgdir}" "${MINGW_PREFIX}"/bin/cmake.exe --install .

  install -Dm644"${srcdir}/${_realname}/LICENSE" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
