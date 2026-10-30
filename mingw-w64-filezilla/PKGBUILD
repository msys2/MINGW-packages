# Maintainer: Andrew Sun <adsun701@gmail.com>

_realname=filezilla
_wx_basever=3.2
pkgbase="mingw-w64-${_realname}"
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}")
# update together with libfilezilla and fzssh
pkgver=3.71.1
pkgrel=1
pkgdesc="Fast and reliable FTP, FTPS and SFTP client (mingw-w64)"
arch=('any')
mingw_arch=('ucrt64' 'clang64' 'clangarm64')
url="https://filezilla-project.org/"
msys2_repository_url="https://svn.filezilla-project.org/filezilla/FileZilla3/"
msys2_documentation_url="https://wiki.filezilla-project.org/Documentation"
msys2_references=(
  'archlinux: filezilla'
  "cpe: cpe:/a:filezilla:filezilla"
  "cpe: cpe:/a:filezilla-project:filezilla_client"
)
license=('spdx:GPL-2.0-or-later')
depends=(
  "${MINGW_PACKAGE_PREFIX}-cc-libs"
  "${MINGW_PACKAGE_PREFIX}-fzssh"
  "${MINGW_PACKAGE_PREFIX}-gnutls"
  "${MINGW_PACKAGE_PREFIX}-libfilezilla"
  "${MINGW_PACKAGE_PREFIX}-sqlite3"
  "${MINGW_PACKAGE_PREFIX}-wxwidgets${_wx_basever}-msw-libs"
)
makedepends=(
  "${MINGW_PACKAGE_PREFIX}-boost"
  "${MINGW_PACKAGE_PREFIX}-cc"
  "${MINGW_PACKAGE_PREFIX}-autotools"
  "${MINGW_PACKAGE_PREFIX}-wxwidgets${_wx_basever}-msw"
)
# Upstream download link became (purposely) unstable
# See https://trac.filezilla-project.org/ticket/13186
#source=("https://download.filezilla-project.org/client/${_realname}_${pkgver}_src.tar.xz"
source=(
  "https://sources.archlinux.org/other/${_realname}/${_realname}-${pkgver}.tar.xz"
  "0002-fix-mingw-compiler-detection.patch"
  "0003-fix-building-po-files.patch"
)
sha256sums=('3df9b9b3e5b2c37dc6d9ecb883554aba140a02ec47c3d6b52333bf1733af4da4'
            'b19da8c59e9010cd08034090a64348d30e2e9d14a8cefa87da994825197f58ba'
            '5d6a473c76366b70e6f5a6581ba86ebf0c6c777d0e0e5385fc0a35ce393b2538')

_apply_patch_with_msg() {
  for _patch in "$@"; do
    msg2 "Applying ${_patch}"
    patch -Np1 -i "${srcdir}/${_patch}"
  done
}

prepare() {
  cd "${_realname}-${pkgver}"

  _apply_patch_with_msg \
    0002-fix-mingw-compiler-detection.patch \
    0003-fix-building-po-files.patch

  autoreconf -fiv
}

build() {
  mkdir -p "build-${MSYSTEM}" && cd "build-${MSYSTEM}"

  if [[ ${MINGW_PACKAGE_PREFIX} != *-clang-* ]]; then
    CFLAGS+=" -Wno-incompatible-pointer-types"
    CXXFLAGS+=" -Wno-incompatible-pointer-types"
  fi

  "../${_realname}-${pkgver}/configure" \
    --prefix="${MINGW_PREFIX}" \
    --build="${MINGW_CHOST}" \
    --host="${MINGW_CHOST}" \
    --target="${MINGW_CHOST}" \
    --disable-manualupdatecheck \
    --disable-autoupdatecheck \
    --with-pugixml=builtin \
    --with-wx-config="${MINGW_PREFIX}/bin/wx-config-${_wx_basever}"

  make
}

check() {
  cd "build-${MSYSTEM}"

  # LANG needed to fix string_test::test_conversion2
  LANG=en_US.UTF-8 make check || true
}

package() {
  cd "build-${MSYSTEM}"

  make install DESTDIR="${pkgdir}"

  cd "${srcdir}/${_realname}-${pkgver}"
  install -Dm644 COPYING -t "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}"
}
