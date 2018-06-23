# Maintainer: Masahiro Sakai <masahiro.sakai@gmail.com>

_realname=mkl-dnn
_mklversion=2018.0.3.20180406
_mklpackage=mklml_win_${_mklversion}
pkgbase=mingw-w64-${_realname}
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"
pkgver=0.14
pkgrel=2
pkgdesc="MKL-DNN - Intel(R) Math Kernel Library for Deep Neural Networks (mingw-w64)"
arch=('any')
url='https://github.com/intel/mkl-dnn'
license=('Apache License 2.0')
depends=("${MINGW_PACKAGE_PREFIX}-gcc-libs")
makedepends=("${MINGW_PACKAGE_PREFIX}-gcc" "${MINGW_PACKAGE_PREFIX}-cmake")
options=('staticlibs' 'strip')
source=(${_realname}-${pkgver}.tar.gz::https://github.com/intel/mkl-dnn/archive/v${pkgver}.tar.gz
        ${_mklpackage}.zip::https://github.com/intel/mkl-dnn/releases/download/v${pkgver}/${_mklpackage}.zip
        0001-mkl-dnn-mingw.patch)
sha256sums=('efebc53882856afec86457a2da644693f5d59c68772d41d640d6b60a8efc4eb0'
            'a584a5bf1c8d2ad70b90d12b52652030e9a338217719064fdb84b7ad0d693694'
            'd5f977699aac072d409320394d87f4c23cc35f0a9930be66f5ba587f59c4d5bd')

prepare() {
  cd "${srcdir}/${_realname}-${pkgver}"
  patch -p1 -i "${srcdir}/0001-mkl-dnn-mingw.patch"
  [[ -d external ]] && rm -rf external
  mkdir -p external
  cp -a ${srcdir}/${_mklpackage} external/
}

build() {
  [[ -d "${srcdir}/build-${MINGW_CHOST}" ]] && rm -rf "${srcdir}/build-${MINGW_CHOST}"
  mkdir "${srcdir}/build-${MINGW_CHOST}"
  cd "${srcdir}/build-${MINGW_CHOST}"
  MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=" \
  ${MINGW_PREFIX}/bin/cmake.exe \
    -G "MSYS Makefiles" \
    -DCMAKE_INSTALL_PREFIX=${MINGW_PREFIX} \
    "${srcdir}/${_realname}-${pkgver}"
  make -j1
}

package() {
  cd "${srcdir}/build-${MINGW_CHOST}"
  make DESTDIR="${pkgdir}" install
  install -Dm644 -t "${pkgdir}${MINGW_PREFIX}/share/doc/mklml" "${srcdir}/${_realname}-${pkgver}/external/${_mklpackage}/license.txt"
}
