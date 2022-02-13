# Maintainer: Some One <some.one@some.email.com>

_realname=somepackage
pkgbase=mingw-w64-python-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-python-${_realname}")
pkgver=1.0.0
pkgrel=1
pkgdesc="Some package (mingw-w64)"
arch=('any')
mingw_arch=('mingw32' 'mingw64' 'ucrt64' 'clang64' 'clang32' 'clangarm64')
url='https://www.somepackage.org/'
license=('LICENSE')
depends=("${MINGW_PACKAGE_PREFIX}-python")
makedepends=("${MINGW_PACKAGE_PREFIX}-python-build" "${MINGW_PACKAGE_PREFIX}-python-installer")
checkdepends=("${MINGW_PACKAGE_PREFIX}-python-pytest")
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
  msg "Python build for ${MSYSTEM}"
  cd "${srcdir}"
  cp -r "${_realname}-${pkgver}" "python-build-${MSYSTEM}" && cd "python-build-${MSYSTEM}"

  ${MINGW_PREFIX}/bin/python -m build --wheel --skip-dependency-check --no-isolation
}

check() {
  msg "Python test for ${MSYSTEM}"
  cd "${srcdir}/python-build-${MSYSTEM}"

# The test command will usually depend upon what is contained in the tox.ini file
# or in the [testenv:py] section of the pyproject.toml file.
  ${MINGW_PREFIX}/bin/python -m pytest
}

package() {
  msg "Python install for ${MSYSTEM}"
  cd "${srcdir}/python-build-${MSYSTEM}"

  MSYS2_ARG_CONV_EXCL="--prefix=" \
    ${MINGW_PREFIX}/bin/python -m installer --prefix=${MINGW_PREFIX} \
    --destdir="${pkgdir}" dist/*.whl

  install -Dm644 LICENSE "${pkgdir}${MINGW_PREFIX}/share/licenses/python-${_realname}/LICENSE"
}
