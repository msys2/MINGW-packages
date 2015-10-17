# Maintainer: Jeroen Ooms <jeroenooms@gmail.com>

_realname=v8
pkgbase=mingw-w64-${_realname}-315
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}-315"
pkgver=3.15.11.18
pkgrel=1
pkgdesc="Fast and modern Javascript engine (mingw-w64)"
arch=('any')
url="http://code.google.com/p/v8"
license=("BSD")
makedepends=("gyp-svn")
depends=("${MINGW_PACKAGE_PREFIX}-readline"
         "${MINGW_PACKAGE_PREFIX}-icu")
options=('!emptydirs' '!strip')
conflicts=("${MINGW_PACKAGE_PREFIX}-v8")
source=("https://github.com/v8/v8/archive/${pkgver}.tar.gz")
md5sums=('1406770373cc26e5af26e5e04c2756f2')

prepare() {
  cd ${_realname}-${pkgver}
  sed -i s/winmm.lib/winmm/g tools/gyp/v8.gyp
  sed -i s/ws2_32.lib/ws2_32/g tools/gyp/v8.gyp
  sed -i '/#include "cctest.h"/a #include "win32-headers.h"' test/cctest/test-platform-win32.cc
}

build() {
  local BUILDTYPE=Release
  local BUILDDIR="${srcdir}/build-${CARCH}"
  export PYTHON=/usr/bin/python2
  mkdir -p "${BUILDDIR}"

  case ${CARCH} in
    i686)
      _arch=ia32
    ;;
    x86_64)
      _arch=x64
    ;;
  esac
  cd "${srcdir}/${_realname}-${pkgver}"
  
  GYP_GENERATORS=make \
    $PYTHON build/gyp_v8 \
      -Dv8_enable_i18n_support=true \
      -Duse_system_icu=1 \
      -Dconsole=readline \
      -Dv8_target_arch=${_arch} \
      --generator-output=${BUILDDIR} \
      -f make

  LINK=g++ make -C ${BUILDDIR} BUILDTYPE=${BUILDTYPE} mksnapshot V=1
  LINK=g++ make -C ${BUILDDIR} BUILDTYPE=${BUILDTYPE} V=1
}

package() {
  cd "${_realname}-${pkgver}"
  local BUILDTYPE=Release
  local BUILDDIR="${srcdir}/build-${CARCH}"

  install -d "$pkgdir"/${MINGW_PREFIX}/bin
  install -Dm755 ${BUILDDIR}/out/${BUILDTYPE}/d8 "$pkgdir"/${MINGW_PREFIX}/bin/d8

  install -d "$pkgdir"/${MINGW_PREFIX}/lib
  install -Dm755 ${BUILDDIR}/out/${BUILDTYPE}/obj.target/tools/gyp/*.a "$pkgdir"/${MINGW_PREFIX}/lib/

  install -d "$pkgdir"/${MINGW_PREFIX}/include
  install -Dm644 include/*.h "$pkgdir"/${MINGW_PREFIX}/include

  install -d "$pkgdir"/${MINGW_PREFIX}/share/licenses/v8
  install -m644 LICENSE* "$pkgdir"/${MINGW_PREFIX}/share/licenses/v8
}
