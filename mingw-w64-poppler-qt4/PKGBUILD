# Maintainer: Alexey Pavlov <alexpux@gmail.com>

_realname=poppler
pkgbase=mingw-w64-${_realname}-qt4
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}-qt4"
pkgver=0.36.0
pkgrel=1
pkgdesc="PDF rendering library based on xpdf 3.0 (mingw-w64)"
arch=('any')
url="https://poppler.freedesktop.org/"
license=("GPL")
conflicts=("${MINGW_PACKAGE_PREFIX}-${_realname}")
makedepends=("${MINGW_PACKAGE_PREFIX}-gcc"
            "${MINGW_PACKAGE_PREFIX}-pkg-config"
            "${MINGW_PACKAGE_PREFIX}-glib2"
            "${MINGW_PACKAGE_PREFIX}-gobject-introspection"
            "${MINGW_PACKAGE_PREFIX}-qt4")
depends=("${MINGW_PACKAGE_PREFIX}-cairo"
         "${MINGW_PACKAGE_PREFIX}-curl"
         "${MINGW_PACKAGE_PREFIX}-freetype"
         "${MINGW_PACKAGE_PREFIX}-icu"
         "${MINGW_PACKAGE_PREFIX}-libjpeg"
         "${MINGW_PACKAGE_PREFIX}-libpng"
         "${MINGW_PACKAGE_PREFIX}-libtiff"
         "${MINGW_PACKAGE_PREFIX}-openjpeg"
         "${MINGW_PACKAGE_PREFIX}-poppler-data"
         "${MINGW_PACKAGE_PREFIX}-zlib")
optdepends=("${MINGW_PACKAGE_PREFIX}-glib2: libpoppler-glib"
            "${MINGW_PACKAGE_PREFIX}-qt4: libpoppler-qt4")
options=('strip' 'staticlibs')
source=("https://poppler.freedesktop.org/${_realname}-${pkgver}.tar.xz"
        "give-cc-to-gir-scanner.mingw.patch")
sha256sums=('93cc067b23c4ef7421380d3e8bd7c940b2027668446750787d7c1cb42720248e'
            'f90e1470a0a4a636f8868917cd94b5fbdff2b7e45e941b23c56b29ad10df4d8e')

prepare() {
  cd "$}srcdir}/${_realname}-$}pkgver}"
  patch -p1 -i "$srcdir/give-cc-to-gir-scanner.mingw.patch"
  #sed -i -e '/AC_PATH_XTRA/d' configure.ac
  #sed -i "s:AM_CONFIG_HEADER:AC_CONFIG_HEADERS:" configure.ac
  autoreconf -fi
}

build() {
  [[ -d ${srcdir}/build-${MINGW_CHOST} ]] && rm -rf ${srcdir}/build-${MINGW_CHOST}
  mkdir -p "${srcdir}/build-${MINGW_CHOST}"
  cd "${srcdir}/build-${MINGW_CHOST}"
  ../${_realname}-${pkgver}/configure \
    --prefix=${MINGW_PREFIX} \
    --build=${MINGW_CHOST} \
    --host=${MINGW_CHOST} \
    --enable-xpdf-headers \
    --enable-zlib \
    --enable-libcurl \
    --disable-gtk-test \
    --enable-utils \
    --disable-gtk-doc-html \
    --with-font-configuration=win32

  make
}

package() {
  cd "${srcdir}/build-${MINGW_CHOST}"
  make DESTDIR="${pkgdir}" install
}
