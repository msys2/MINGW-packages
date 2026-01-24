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
makedepends=("${MINGW_PACKAGE_PREFIX}-autotools"
             "${MINGW_PACKAGE_PREFIX}-cc")
source=("https://www.somepackage.org/${_realname}/${_realname}-${pkgver}.tar.gz"
        "0001-A-fix.patch")
sha256sums=('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb')

prepare() {
  cd "${_realname}-${pkgver}"

  patch -p1 -i ../0001-A-fix.patch
}

build() {
  mkdir -p "build-${MSYSTEM}" && cd "build-${MSYSTEM}"

  ../"${_realname}-${pkgver}"/configure \
    --prefix="${MINGW_PREFIX}" \
    --build="${MINGW_CHOST}" \
    --host="${MINGW_CHOST}" \
    --target="${MINGW_CHOST}" \
    --enable-static \
    --enable-shared

  make
}

check() {
  cd "build-${MSYSTEM}"

  make check
}

package() {
  cd "build-${MSYSTEM}"

  make install DESTDIR="${pkgdir}"

  install -Dm644 "../${_realname}-${pkgver}/LICENSE" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
