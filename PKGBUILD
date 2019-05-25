# Maintainer: Alexey Pavlov <alexpux@gmail.com>
# Contributor: Ray Donnelly <mingw.android@gmail.com>

_realname=gdb
pkgbase=mingw-w64-${_realname}
pkgname="${MINGW_PACKAGE_PREFIX}-${_realname}"
# Until https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62279 is
# fixed, we will stick with 7.6.2 which hasn't got this bug..
pkgver=8.3
pkgrel=2
pkgdesc="GNU Debugger (mingw-w64)"
arch=('any')
url="https://www.gnu.org/software/gdb/"
license=('GPL')
groups=("${MINGW_PACKAGE_PREFIX}-toolchain")
depends=("${MINGW_PACKAGE_PREFIX}-expat"
         "${MINGW_PACKAGE_PREFIX}-libiconv"
         "${MINGW_PACKAGE_PREFIX}-python3"
         "${MINGW_PACKAGE_PREFIX}-readline"
         "${MINGW_PACKAGE_PREFIX}-zlib")
checkdepends=('dejagnu' 'bc')
makedepends=("${MINGW_PACKAGE_PREFIX}-iconv"
             "${MINGW_PACKAGE_PREFIX}-ncurses"
             "${MINGW_PACKAGE_PREFIX}-xz")
options=('staticlibs' '!distcc' '!ccache')
source=(https://ftp.gnu.org/gnu/gdb/${_realname}-${pkgver}.tar.xz{,.sig}
        'gdb-fix-display-tabs-on-mingw.patch'
        #'gdb-mingw-gcc-4.7.patch'
        'gdb-7.8.1-mingw-gcc-4.7.patch'
        'gdb-7.6.2-mingw-gcc-4.7.patch'
        'gdb-perfomance.patch'
        'gdb-fix-using-gnu-print.patch'
        'gdb-7.12-dynamic-libs.patch'
        'gdb-py3-fixes.patch'
        'python-configure-path-fixes.patch')
validpgpkeys=('F40ADB902B24264AA42E50BF92EDB04BFF325CF3')
sha256sums=('802f7ee309dcc547d65a68d61ebd6526762d26c3051f52caebe2189ac1ffd72e'
            'SKIP'
            'a3e64bf3acc8e92016d493aa1e2fbb5b55c67bfce4359b3578d7a6b5e5d5692a'
            '225608a1e486ee98641611804b07209915367c6767a735ae928175ad45b93060'
            '82ea717b628c8970055a9cbb8edaed8fbebc2cac27467337426dfc4b4c514a6b'
            '6be0f95483e02d5447c93ab5e2c9554d254c6888b1fa3f3a4c011cd6a0a403ad'
            '264eba33ef71c4762514a634dcf94fdb778914dbba6ce99e63804fe063225d97'
            '4398bd83a798baa15c0f91878391a0882239a682cf523ef6551766cba7b03699'
            'bfe8985e806200e5a1007c4565bd60de0a297369d17d4a0178512f2df29b34fc'
            'bdf6c2b51d6c8c84b7b20abb9d1702f110c0c0e06e3d68eb6aba817664354450')

prepare() {
  cd ${srcdir}/${_realname}-${pkgver}

  # https://sourceware.org/ml/gdb-patches/2013-11/msg00224.html
  #patch -p1 -i ${srcdir}/gdb-fix-display-tabs-on-mingw.patch
  # https://sourceware.org/bugzilla/show_bug.cgi?id=15559
  # patch -p1 -i ${srcdir}/gdb-mingw-gcc-4.7.patch
  # patch -p1 -i ${srcdir}/gdb-${pkgver}-mingw-gcc-4.7.patch
  # https://sourceware.org/bugzilla/show_bug.cgi?id=15412
  patch -p1 -i ${srcdir}/gdb-perfomance.patch

  patch -p1 -i ${srcdir}/gdb-fix-using-gnu-print.patch

  # https://sourceware.org/bugzilla/show_bug.cgi?id=21078
  patch -p1 -i ${srcdir}/gdb-7.12-dynamic-libs.patch

  patch -p1 -i ${srcdir}/gdb-py3-fixes.patch
  patch -p1 -i ${srcdir}/python-configure-path-fixes.patch

  # hack! - libiberty configure tests for header files using "$CPP $CPPFLAGS"
  sed -i "/ac_cpp=/s/\$CPPFLAGS/\$CPPFLAGS -O2/" libiberty/configure
}

build() {
  [[ -d ${srcdir}/build-${MINGW_CHOST} ]] && rm -rf ${srcdir}/build-${MINGW_CHOST}
  mkdir ${srcdir}/build-${MINGW_CHOST}
  cd ${srcdir}/build-${MINGW_CHOST}

  ../${_realname}-${pkgver}/configure \
    --build=${MINGW_CHOST} \
    --host=${MINGW_CHOST} \
    --target=${MINGW_CHOST} \
    --prefix=${MINGW_PREFIX} \
    --enable-targets="i686-w64-mingw32,x86_64-w64-mingw32" \
    --enable-64-bit-bfd \
    --disable-werror \
    --disable-win32-registry \
    --disable-rpath \
    --with-system-gdbinit=${MINGW_PREFIX}/etc/gdbinit \
    --with-system-readline \
    --with-python=${MINGW_PREFIX}/bin/python3 \
    --with-expat \
    --with-libiconv-prefix=${MINGW_PREFIX} \
    --with-zlib \
    --with-lzma \
    --enable-tui \
    --enable-source-highlight=no

  make
}

package() {
  cd ${srcdir}/build-${MINGW_CHOST}
  make DESTDIR=${pkgdir} install

  # Remove unwanted files
  rm -rf ${pkgdir}${MINGW_PREFIX}/share/{man,info}

  rm -f ${pkgdir}${MINGW_PREFIX}/include/*.h
  rm -f ${pkgdir}${MINGW_PREFIX}/lib/*.a
}
