# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-python-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-python-${_realname}")
pkgver=1.0.0
pkgrel=1
pkgdesc="Some package (mingw-w64)"
arch=('any')
url='https://www.somepackage.org/'
license=('LICENSE')
depends=("${MINGW_PACKAGE_PREFIX}-python")
makedepends=("${MINGW_PACKAGE_PREFIX}-python"
             "${MINGW_PACKAGE_PREFIX}-python-setuptools")
source=("https://pypi.org/packages/source/${_realname::1}/${_realname}/${_realname}-${pkgver}.tar.gz"
        "0001-An-important-fix.patch"
        "0002-A-less-important-fix.patch")
sha256sums=('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
            'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
            'cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc')

prepare() {
  cd "${srcdir}/${_realname}-${pkgver}"

  patch -Np1 -i "${srcdir}/0001-A-really-important-fix.patch"
  patch -Np1 -i "${srcdir}/0002-A-less-important-fix.patch"

  ## (OPTIONAL) Only if setuptools-scm is used
  # Set version for setuptools_scm
  export SETUPTOOLS_SCM_PRETEND_VERSION=${pkgver}
}

build() {
  cd "${srcdir}"
  cp -r "${_realname}-${pkgver}" "python-build-${MSYSTEM}" && cd "python-build-${MSYSTEM}"

  "${MINGW_PREFIX}"/bin/python setup.py build
}

check() {
  cd "${srcdir}/python-build-${MSYSTEM}"

  "${MINGW_PREFIX}"/bin/python setup.py check
}

package() {
  cd "${srcdir}/python-build-${MSYSTEM}"

  MSYS2_ARG_CONV_EXCL="--prefix=;--install-scripts=;--install-platlib=" \
    "${MINGW_PREFIX}"/bin/python setup.py install --prefix="${MINGW_PREFIX}" \
      --root="${pkgdir}" --optimize=1 --skip-build

  install -Dm644 LICENSE "${pkgdir}${MINGW_PREFIX}/share/licenses/python-${_realname}/LICENSE"

  ## (OPTIONAL) This entire section should be removed
  ## if the package does NOT instal anything in the ${MINGW_PREFIX}/bin directory.
  for _f in "${pkgdir}${MINGW_PREFIX}"/bin/*-script.py; do
    # Remove shebang line
    sed -e '1 { s/^#!.*$// }' -i "${_f}"
  done
}
