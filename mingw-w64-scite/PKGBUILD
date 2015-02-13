# Maintainer: David Macek <david.macek.0@gmail.com>

_realname=scite
pkgname=("${MINGW_PACKAGE_PREFIX}-${_realname}"
         "${MINGW_PACKAGE_PREFIX}-${_realname}-defaults")
pkgver=3.5.3
pkgrel=2
arch=('any')
url='http://www.scintilla.org/SciTE.html'
license=('custom:scite')
makedepends=("${MINGW_PACKAGE_PREFIX}-gcc"
             "${MINGW_PACKAGE_PREFIX}-gtk3"
             "${MINGW_PACKAGE_PREFIX}-pkg-config")
source=("http://downloads.sourceforge.net/scintilla/${_realname}${pkgver//./}.tgz"
        "0001-Use-POSIX-tools-in-makefiles.patch"
        "0002-Prefix-library-names.patch"
        "0003-Use-FHS.patch")
md5sums=('44df4b3e25d346189ddc688c689dcf05'
         'd4bd88fa3e173d26d9cacaef12b1d6f9'
         '2a9ec192f43f82d4b26b4a6667de350a'
         '17011da4ced7269d787433b599a1ae92')

prepare() {
  cd "${srcdir}"
  patch -p1 -i "${srcdir}"/0001-Use-POSIX-tools-in-makefiles.patch
  patch -p1 -i "${srcdir}"/0002-Prefix-library-names.patch
  patch -p1 -i "${srcdir}"/0003-Use-FHS.patch
  mkdir -p "${srcdir}"/build-${CARCH}
  cp -r "${srcdir}"/{scite,scintilla} "${srcdir}"/build-${CARCH}
}

build() {
  cd "${srcdir}"/build-${CARCH}
  
  GTK3=1 make -C scintilla/gtk
  make -C scintilla/win32
  # GTK3=1 make -C scite/gtk
  make -C scite/win32

}

package_mingw-w64-scite() {
  pkgdesc='Editor with facilities for building and running programs (mingw-w64)'
  depends=("${MINGW_PACKAGE_PREFIX}-glib2"
           "${MINGW_PACKAGE_PREFIX}-gtk3")
  optdepends=("${MINGW_PACKAGE_PREFIX}-${_realname}-defaults: Default language files")

  cd "${srcdir}"/build-${CARCH}

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/bin
  cp scintilla/bin/lib{Scintilla,SciLexer}.dll "${pkgdir}"${MINGW_PREFIX}/bin
  cp scite/bin/SciTE.exe "${pkgdir}"${MINGW_PREFIX}/bin

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/lib
  cp scintilla/bin/libScintillaGtk.a "${pkgdir}"${MINGW_PREFIX}/lib

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/include/scintilla
  cp scintilla/include/{*.h,*.iface} "${pkgdir}"${MINGW_PREFIX}/include/scintilla

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/share/scite
  cp scite/bin/SciTEGlobal.properties "${pkgdir}"${MINGW_PREFIX}/share/scite

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/share/doc/scite
  cp scite/doc/{*.png,*.jpg,*.html} "${pkgdir}"${MINGW_PREFIX}/share/doc/scite

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/share/man/man1
  cp scite/doc/scite.1 "${pkgdir}"${MINGW_PREFIX}/share/man/man1/SciTE.1

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/share/licenses/scite
  cp scintilla/License.txt "${pkgdir}"${MINGW_PREFIX}/share/licenses/scite/LICENSE-scintilla
  cp scite/License.txt "${pkgdir}"${MINGW_PREFIX}/share/licenses/scite/LICENSE-scite
  cp scite/lua/COPYRIGHT "${pkgdir}"${MINGW_PREFIX}/share/licenses/scite/LICENSE-lua
}

package_mingw-w64-scite-defaults() {
  pkgdesc='Default language files for the SciTE editor (mingw-w64)'
  depends=("${MINGW_PACKAGE_PREFIX}-${_realname}")

  cd "${srcdir}"/build-${CARCH}

  mkdir -p "${pkgdir}"${MINGW_PREFIX}/share/scite
  cp scite/bin/*.properties "${pkgdir}"${MINGW_PREFIX}/share/scite
  rm -f "${pkgdir}"${MINGW_PREFIX}/share/scite/SciTEGlobal.properties
}

package_mingw-w64-i686-scite() {
  package_mingw-w64-scite
}

package_mingw-w64-i686-scite-defaults() {
  package_mingw-w64-scite-defaults
}

package_mingw-w64-x86_64-scite() {
  package_mingw-w64-scite
}

package_mingw-w64-x86_64-scite-defaults() {
  package_mingw-w64-scite-defaults
}
