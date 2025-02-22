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
makedepends=("${MINGW_PACKAGE_PREFIX}-rust"
             "${MINGW_PACKAGE_PREFIX}-cargo-c")
source=("https://www.somepackage.org/${_realname}/${_realname}-${pkgver}.tar.gz"
        "0001-An-important-fix.patch"
        "0002-A-less-important-fix.patch")
sha256sums=('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
            'cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc')

prepare() {
  cd "${_realname}-${pkgver}"

  patch -Np1 -i "${srcdir}"/0001-A-really-important-fix.patch
  patch -Np1 -i "${srcdir}"/0002-A-less-important-fix.patch

  cargo fetch --locked
}

build() {
  cd "${_realname}-${pkgver}"

  MSYS2_ARG_CONV_EXCL="--prefix=" \
    cargo cbuild \
      --meson-paths \
      --release \
      --frozen \
      --all-features \
      --prefix="${MINGW_PREFIX}"
}

check() {
  cd "${_realname}-${pkgver}"

  cargo test --release --frozen
}

package() {
  cd "${_realname}-${pkgver}"

  MSYS2_ARG_CONV_EXCL="--prefix=" \
    cargo cinstall \
      --meson-paths \
      --release \
      --frozen \
      --all-features \
      --prefix="${MINGW_PREFIX}" \
      --destdir="${pkgdir}"

  # Remove def file
  rm -f "${pkgdir}"${MINGW_PREFIX}/lib/*.def

  install -Dm644 LICENSE "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
