# Contributor (Fluke): Ricky Wu <rickleaf.wu@gmail.com>

_realname=lsqlite3
pkgbase=mingw-w64-${_realname}
pkgname=("${MINGW_PACKAGE_PREFIX}-lua51-${_realname}")
pkgver=0.9.3
pkgrel=1
pkgdesc="LuaSQLite is a Lua 5 binding to allow users/developers to manipulate SQLite 2 and SQLite 3 databases (through different implementations) from lua"
arch=('any')
url="http://lua.sqlite.org"
license=("LGPL")
depends=(
         "${MINGW_PACKAGE_PREFIX}-lua51"
         "${MINGW_PACKAGE_PREFIX}-sqlite3"
        )
makedepends=(
             "${MINGW_PACKAGE_PREFIX}-gcc")
             
options=('staticlibs' 'strip')
source=("http://lua.sqlite.org/index.cgi/zip/${_realname}_fsl09w.zip"
		"installpath.lua"
		"lunit.lua"
		"Makefile.mingw"
		)

sha256sums=('795319d59d227a5c5471eda132f4cdff86eb2643d9b42198c81aa09ba89c96fc'
			'd9b058b10d1d6e0fc1ac9338193c8b8b25e808e00955ce13ef1c91c6af0c9d3a'
			'ae756636ed73147259cb55e13768d88bef5e7fa3373209c886ab46e8ea89b92f'
			'1e2980df7b8dfff693f401a3f222076b150d6f2f2f992bdf690982235885db42'
			)

prepare() {
  cd "${srcdir}/${_realname}_fsl09w"
  cp ../installpath.lua .
  cp ../lunit.lua .
  cp ../Makefile.mingw .
  mkdir -p tmp
}

build() {
  #mkdir -p "build-${MINGW_CHOST}"
  cd "${srcdir}/${_realname}_fsl09w"

  make -f Makefile.mingw
}

package() {
  cd "${srcdir}/${_realname}_fsl09w"
  install -Dm755 lsqlite3.dll "${pkgdir}${MINGW_PREFIX}/lib/lua/5.1/lsqlite3.dll"
}
