# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-${_realname}-git
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}-git"
provides="${MINGW_PACKAGE_PREFIX}-${_realname}"
replaces="${MINGW_PACKAGE_PREFIX}-${_realname}"
pkgver=100.77777777
pkgrel=1
pkgdesc="Some package (git) (mingw-w64)"
arch=('any')
url="https://github.com/someproject/somepackage"
license=('LICENSE')
validpgpkeys=('gpg_KEY')
makedepends=("${MINGW_PACKAGE_PREFIX}-cmake"
             "${MINGW_PACKAGE_PREFIX}-ninja"
             'git')
source=("${_realname}"::"git+https://github.com/someproject/somepackage.git#branch=master"
        0001-An-important-fix.patch
        0002-A-less-important-fix.patch)
sha256sums=('SKIP'
            'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb')

pkgver() {
  cd "${srcdir}"/${_realname}

  git describe --long | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}

prepare() {
  cd "${srcdir}"/${_realname}

  git apply "${srcdir}"/0001-A-really-important-fix.patch
  git apply "${srcdir}"/0002-A-less-important-fix.patch
}

build() {
  [[ -d "${srcdir}"/build-${MSYSTEM} ]] && rm -rf "${srcdir}"/build-${MSYSTEM}
  mkdir -p "${srcdir}"/build-${MSYSTEM} && cd "${srcdir}"/build-${MSYSTEM}

  declare -a extra_config
  if check_option "debug" "n"; then
    extra_config+=("-DCMAKE_BUILD_TYPE=Release")
  else
    extra_config+=("-DCMAKE_BUILD_TYPE=Debug")
  fi

  MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=" \
    ${MINGW_PREFIX}/bin/cmake.exe \
      -GNinja \
      -DCMAKE_INSTALL_PREFIX=${MINGW_PREFIX} \
      "${extra_config[@]}" \
      -DBUILD_{SHARED,STATIC}_LIBS=ON \
      ../${_realname}

  ${MINGW_PREFIX}/bin/cmake.exe --build .
}

check() {
  cd "${srcdir}"/build-${MSYSTEM}

  cmake --build . --target test
}

package() {
  cd "${srcdir}"/build-${MSYSTEM}

  DESTDIR="${pkgdir}" ${MINGW_PREFIX}/bin/cmake.exe --install .

  install -Dm644 ${srcdir}/${_realname}/LICENSE ${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE
}
