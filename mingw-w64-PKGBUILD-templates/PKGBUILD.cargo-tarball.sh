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
makedepends=("${MINGW_PACKAGE_PREFIX}-rust")
source=("https://www.somepackage.org/${_realname}/${_realname}-${pkgver}.tar.gz"
        "0001-An-important-fix.patch"
        "0002-A-less-important-fix.patch")
sha256sums=('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
            'cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc')

prepare() {
  cd "${srcdir}/${_realname}-${pkgver}"

  patch -Np1 -i "${srcdir}"/0001-A-really-important-fix.patch
  patch -Np1 -i "${srcdir}"/0002-A-less-important-fix.patch

  cargo fetch --locked
}

build() {
  cd "${srcdir}/${_realname}-${pkgver}"

  cargo build --release --frozen
}

check() {
  cd "${srcdir}/${_realname}-${pkgver}"

  cargo test --release --frozen
}

package() {
  cd "${srcdir}/${_realname}-${pkgver}"

  cargo install \
    --offline \
    --no-track \
    --frozen \
    --path . \
    --root "${pkgdir}${MINGW_PREFIX}"

  install -Dm644 "${srcdir}/${_realname}-${pkgver}/LICENSE" "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
