# Contributor: Chocobo1 <https://github.com/Chocobo1>

_realname=qbittorrent
pkgbase=mingw-w64-${_realname}-git
pkgname=${MINGW_PACKAGE_PREFIX}-${_realname}-git
pkgver=r8129.d821bdc9f
pkgrel=1
pkgdesc="An advanced BitTorrent client programmed in C++, based on Qt toolkit and libtorrent-rasterbar. (mingw-w64)"
arch=('any')
url="https://qbittorrent.org/"
license=('custom' 'GPL')
depends=("${MINGW_PACKAGE_PREFIX}-qt5"
         "${MINGW_PACKAGE_PREFIX}-libtorrent-rasterbar")
makedepends=("git")
optdepends=("${MINGW_PACKAGE_PREFIX}-python3: needed for torrent search tab")
provides=("${MINGW_PACKAGE_PREFIX}-${_realname}")
conflicts=("${MINGW_PACKAGE_PREFIX}-${_realname}")
source=("git+https://github.com/qbittorrent/qBittorrent.git")
sha256sums=('SKIP')


prepare() {
  cd "${srcdir}/${_realname}"

  # prepare env for mingw
  sed -i 's/unix:!macx:/unix|win32-g++:/g' "src/src.pro"
  sed -i 's/!haiku/#!haiku/g' "unixconf.pri"
}

pkgver() {
  cd "${srcdir}/${_realname}"

  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/${_realname}"

  ./bootstrap.sh
  ./configure --prefix=${MINGW_PREFIX} \
    --build=${MINGW_CHOST} --host=${MINGW_CHOST} --target=${MINGW_CHOST} \
    --with-boost-system=boost_system-mt
  make
}

package() {
  cd "$srcdir/${_realname}"

  make INSTALL_ROOT=${pkgdir} install
  install -Dm644 "COPYING" "${pkgdir}${MINGW_PREFIX}/usr/share/licenses/${_realname}/COPYING"
}
