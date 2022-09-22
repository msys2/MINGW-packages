# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}")
pkgver=1.0
pkgrel=1
pkgdesc="Some package (mingw-w64)"
arch=('any')
mingw_arch=('mingw32' 'mingw64' 'ucrt64' 'clang64' 'clang32')
url='https://www.somepackage.org/'
license=('LICENSE')
makedepends=("${MINGW_PACKAGE_PREFIX}-cmake"
             "${MINGW_PACKAGE_PREFIX}-ninja"
             "${MINGW_PACKAGE_PREFIX}-cc")
source=("https://www.somepackage.org/${_realname}/${_realname}-${pkgver}.tar.gz"
        "0001-An-important-fix.patch"
        "0002-A-less-important-fix.patch")
sha256sums=('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
            'cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc')

prepare() {
  cd "${srcdir}"/${_realname}-${pkgver}

  patch -Np1 -i "${srcdir}"/0001-A-really-important-fix.patch
  patch -Np1 -i "${srcdir}"/0002-A-less-important-fix.patch
}

build() {
  cd "${srcdir}/${_realname}-${pkgver}"
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
      ../${_realname}-${pkgver}

  "${MINGW_PREFIX}"/bin/cmake.exe --build .
}

check() {
  cd "${srcdir}/build-${MSYSTEM}"

  "${MINGW_PREFIX}"/bin/cmake.exe --build . --target test
}

package() {
  cd "${srcdir}/build-${MSYSTEM}"

  DESTDIR="${pkgdir}" "${MINGW_PREFIX}"/bin/cmake.exe --install .

  install -Dm644 "${srcdir}/${_realname}-${pkgver}/LICENSE" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
