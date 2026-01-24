# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}")
pkgver=1.0
pkgrel=1
pkgdesc="Some package (mingw-w64)"
arch=('any')
mingw_arch=('mingw64' 'ucrt64' 'clang64' 'clangarm64')
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
  cd "${_realname}-${pkgver}"

  patch -Np1 -i ../0001-A-really-important-fix.patch
  patch -Np1 -i ../0002-A-less-important-fix.patch
}

build() {
  declare -a extra_config
  if check_option "debug" "n"; then
    extra_config+=("-DCMAKE_BUILD_TYPE=Release")
  else
    extra_config+=("-DCMAKE_BUILD_TYPE=Debug")
  fi

  MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=" \
    cmake \
      -GNinja \
      -DCMAKE_INSTALL_PREFIX="${MINGW_PREFIX}" \
      "${extra_config[@]}" \
      -DBUILD_{SHARED,STATIC}_LIBS=ON \
      -S "${_realname}-${pkgver}" \
      -B "build-${MSYSTEM}"

  cmake --build "build-${MSYSTEM}"
}

check() {
  cmake --build "build-${MSYSTEM}" --target test
}

package() {
  DESTDIR="${pkgdir}" cmake --install "build-${MSYSTEM}"

  install -Dm644 "${_realname}-${pkgver}/LICENSE" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
