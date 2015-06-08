# Maintainer: Maarten de Vries
# Contributors: Dave Reisner <dreisner@archlinux.org>
#               Matthias Blaicher <matthias@blaicher.com>
#               Severen Redwood <severen@shrike.me>

pkgname=mingw-w64-capnproto
pkgver=0.7.0
pkgrel=1
pkgdesc="Cap'n Proto serialization/RPC system"
arch=('i686' 'x86_64')
url='https://capnproto.org/'
license=('MIT')
makedepends=(mingw-w64-configure)
source=("https://capnproto.org/capnproto-c++-${pkgver}.tar.gz")
sha512sums=('9f8fb5753155798fcf9377a87f984a54d9fc5157c41aa11cd94108a773ca22d6e6952657e2d8079c9806f7de06f316c94957329fa52dbab6207aaa3b52348f04')

options=(!strip !buildflags staticlibs)

_architectures=("i686-w64-mingw32" "x86_64-w64-mingw32")

build() {
	env
	cd "$srcdir/capnproto-c++-$pkgver"
	for _arch in ${_architectures[@]}; do
		mkdir -p build-${_arch} && pushd build-${_arch}
		${_arch}-configure --with-external-capnp --disable-shared --enable-static --disable-reflection
		make
		popd
	done
}

package() {
	for _arch in ${_architectures[@]}; do
		cd "${srcdir}/capnproto-c++-${pkgver}/build-${_arch}"
		make DESTDIR="$pkgdir" install
		${_arch}-strip -g "$pkgdir"/usr/${_arch}/lib/*.a
	done
}

# vim:set ts=4 sw=4:
