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
makedepends=("${MINGW_PACKAGE_PREFIX}-meson"
             "${MINGW_PACKAGE_PREFIX}-ninja"
             "${MINGW_PACKAGE_PREFIX}-pkg-config"
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
  mkdir -p build-${MSYSTEM} && cd build-${MSYSTEM}

  MSYS2_ARG_CONV_EXCL="--prefix=" \
    meson \
      --prefix="${MINGW_PREFIX}" \
      --wrap-mode=nodownload \
      --auto-features=enabled \
      --buildtype=plain \
      ../${_realname}-${pkgver}

  meson compile
}

check() {
  cd "${srcdir}/build-${MSYSTEM}"

  meson test
}

package() {
  cd "${srcdir}/build-${MSYSTEM}"

  DESTDIR="${pkgdir}" meson install

  install -Dm644 "${srcdir}/${_realname}-${pkgver}/COPYING" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/COPYING"
}
