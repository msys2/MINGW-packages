# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}")
pkgver=1.0
pkgrel=1
pkgdesc="Some package (mingw-w64)"
arch=('any')
mingw_arch=('ucrt64' 'clang64' 'clangarm64')
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
  cd "${_realname}-${pkgver}"

  patch -Np1 -i ../0001-A-really-important-fix.patch
  patch -Np1 -i ../0002-A-less-important-fix.patch
}

build() {
  MSYS2_ARG_CONV_EXCL="--prefix=" \
    meson setup \
      --prefix="${MINGW_PREFIX}" \
      --wrap-mode=nodownload \
      --auto-features=enabled \
      --buildtype=plain \
      "build-${MSYSTEM}" \
      "${_realname}-${pkgver}"

  meson compile -C "build-${MSYSTEM}"
}

check() {
  meson test -C "build-${MSYSTEM}"
}

package() {
  meson install -C "build-${MSYSTEM}" --destdir "${pkgdir}"

  install -Dm644 "${_realname}-${pkgver}/COPYING" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/COPYING"
}
