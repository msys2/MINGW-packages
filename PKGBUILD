# Maintainer: Antigravity <antigravity@example.com>

_realname=clamav
pkgbase=mingw-w64-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}")
pkgver=1.5.1
pkgrel=1
pkgdesc="Open source antivirus engine (mingw-w64)"
arch=('any')
mingw_arch=('mingw64' 'ucrt64' 'clang64' 'clangarm64')
url='https://www.clamav.net/'
license=('spdx:GPL-2.0-only')
depends=("${MINGW_PACKAGE_PREFIX}-openssl"
         "${MINGW_PACKAGE_PREFIX}-zlib"
         "${MINGW_PACKAGE_PREFIX}-libxml2"
         "${MINGW_PACKAGE_PREFIX}-pcre2"
         "${MINGW_PACKAGE_PREFIX}-json-c"
         "${MINGW_PACKAGE_PREFIX}-bzip2"
         "${MINGW_PACKAGE_PREFIX}-curl"
         "${MINGW_PACKAGE_PREFIX}-libiconv"
         "${MINGW_PACKAGE_PREFIX}-ncurses")
makedepends=("${MINGW_PACKAGE_PREFIX}-cmake"
             "${MINGW_PACKAGE_PREFIX}-ninja"
             "${MINGW_PACKAGE_PREFIX}-cc"
             "${MINGW_PACKAGE_PREFIX}-rust"
             "${MINGW_PACKAGE_PREFIX}-check")
source=("https://github.com/Cisco-Talos/clamav/archive/refs/tags/clamav-${pkgver}.zip"
        "0001-mingw-build-fixes.patch")
sha256sums=('SKIP'
            'SKIP')

prepare() {
  cd "${_realname}-${_realname}-${pkgver}"
  patch -p1 -i "${srcdir}/0001-mingw-build-fixes.patch"
}

build() {
  cd "${_realname}-${_realname}-${pkgver}"
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
      -DBUILD_SHARED_LIBS=ON \
      -DENABLE_TESTS=OFF \
      -DOPTIMIZE=ON \
      -S . \
      -B "../build-${MSYSTEM}"

  cmake --build "../build-${MSYSTEM}"||cmake --build "../build-${MSYSTEM}"cmake --build "../build-${MSYSTEM}"
}

check() {
  cd "${_realname}-${_realname}-${pkgver}"
}

package() {
  cd "${_realname}-${_realname}-${pkgver}"
  DESTDIR="${pkgdir}" cmake --install "../build-${MSYSTEM}"
  
  install -Dm644 COPYING.txt "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/COPYING"
}
