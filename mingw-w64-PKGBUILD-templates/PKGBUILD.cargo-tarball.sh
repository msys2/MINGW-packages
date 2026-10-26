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
makedepends=("${MINGW_PACKAGE_PREFIX}-rust")
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

  # if cargo wants to make an http request at build stage, use `cargo fetch --locked` instead
  cargo fetch --locked --target "${RUST_CHOST}"
}

build() {
  cd "${_realname}-${pkgver}"

  cargo build --release --frozen
}

check() {
  cd "${_realname}-${pkgver}"

  cargo test --release --frozen
}

package() {
  cd "${_realname}-${pkgver}"

  cargo install \
    --offline \
    --no-track \
    --frozen \
    --path . \
    --root "${pkgdir}${MINGW_PREFIX}"

  install -Dm644 LICENSE "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
