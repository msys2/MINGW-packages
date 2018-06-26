# Maintainer: Masahiro Sakai <masahiro.sakai@gmail.com>

_realname=menoh
_onnxversion=1.2.1
pkgbase=mingw-w64-${_realname}
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"
pkgver=1.0.1
pkgrel=1
pkgdesc="Menoh - DNN inference library (mingw-w64)"
arch=('any')
url='https://github.com/pfnet-research/menoh'
license=('MIT License')
depends=("${MINGW_PACKAGE_PREFIX}-mkl-dnn"
          "${MINGW_PACKAGE_PREFIX}-opencv"
          "${MINGW_PACKAGE_PREFIX}-protobuf")
makedepends=("${MINGW_PACKAGE_PREFIX}-gcc"
             "${MINGW_PACKAGE_PREFIX}-cmake"
             "${MINGW_PACKAGE_PREFIX}-mkl-dnn"
             "${MINGW_PACKAGE_PREFIX}-opencv"
             "${MINGW_PACKAGE_PREFIX}-protobuf")
options=('staticlibs' 'strip')
source=(${_realname}-${pkgver}.tar.gz::https://github.com/pfnet-research/menoh/archive/v${pkgver}.tar.gz
        onnx-${_onnxversion}.tar.gz::https://github.com/onnx/onnx/archive/v${_onnxversion}.tar.gz
        0000-remove-dllexport.patch)
sha256sums=('941daa000136d20d6af47f062e1eb8dbc5894a214ce2f553409cd833804e7425'
            'ede43fdcdee6f53ba5110aa55a5996a3be36fe051698887ce311c26c86efacf8'
            '4b72adb5562124cafd716a10e83fd7a83d8254d1a01559d70a456beb4aeacff9')

prepare() {
  cd "${srcdir}/${_realname}-${pkgver}"
  patch -p1 -i ${srcdir}/0000-remove-dllexport.patch
  rm -rf external/onnx
  cp -R ${srcdir}/onnx-${_onnxversion} external/onnx
}

build() {
  [[ -d "${srcdir}/build-${MINGW_CHOST}" ]] && rm -rf "${srcdir}/build-${MINGW_CHOST}"
  mkdir "${srcdir}/build-${MINGW_CHOST}"
  cd "${srcdir}/build-${MINGW_CHOST}"
  MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=;-DMKLDNN_LIBRARY=;-DPROTOBUF_LIBRARY=" \
  ${MINGW_PREFIX}/bin/cmake.exe \
    -G "MSYS Makefiles" \
    -DCMAKE_INSTALL_PREFIX=${MINGW_PREFIX} \
    -DMKLDNN_LIBRARY=${MINGW_PREFIX}/lib/libmkldnn.dll.a \
    -DPROTOBUF_LIBRARY=${MINGW_PREFIX}/lib/libprotobuf.dll.a \
    "${srcdir}/${_realname}-${pkgver}"
  make -j1
}

package() {
  cd "${srcdir}/build-${MINGW_CHOST}"
  make DESTDIR="${pkgdir}" install
  install -Dm644 -t "${pkgdir}${MINGW_PREFIX}/libexec/${_realname}" example/*.exe tool/*.exe benchmark/*.exe

  cd "${srcdir}/${_realname}-${pkgver}"
  install -Dm755 -t "${pkgdir}${MINGW_PREFIX}/share/doc/${_realname}" LICENSE README.md docs/*
}
