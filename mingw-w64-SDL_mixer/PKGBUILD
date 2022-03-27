# Maintainer: Alexey Pavlov <alexpux@gmail.com>

_realname=SDL_mixer
pkgbase=mingw-w64-${_realname}
pkgname=${MINGW_PACKAGE_PREFIX}-${_realname}
pkgver=1.2.12
pkgrel=9
pkgdesc="A simple multi-channel audio mixer (mingw-w64)"
arch=('any')
mingw_arch=('mingw32' 'mingw64' 'ucrt64' 'clang64' 'clang32')
url="https://libsdl.org/projects/SDL_mixer/"
license=('custom')
depends=("${MINGW_PACKAGE_PREFIX}-SDL"
         "${MINGW_PACKAGE_PREFIX}-libvorbis"
         "${MINGW_PACKAGE_PREFIX}-libmikmod"
         "${MINGW_PACKAGE_PREFIX}-libmad"
         "${MINGW_PACKAGE_PREFIX}-flac"
         )
makedepends=("${MINGW_PACKAGE_PREFIX}-cc"
             "${MINGW_PACKAGE_PREFIX}-autotools"
             "${MINGW_PACKAGE_PREFIX}-fluidsynth"
             "${MINGW_PACKAGE_PREFIX}-openal"
             "${MINGW_PACKAGE_PREFIX}-smpeg")
optdepends=("${MINGW_PACKAGE_PREFIX}-fluidsynth: MIDI software synth, replaces built-in timidity")
source=(https://libsdl.org/projects/SDL_mixer/release/${_realname}-${pkgver}.tar.gz
        mikmod1.patch
        mikmod2.patch
        fluidsynth-volume.patch
        double-free-crash.patch
        SDL_mixer-find_lib.mingw.patch
        autoreconf-fix.patch
        non-void-return-value.patch)
sha256sums=('1644308279a975799049e4826af2cfc787cad2abb11aa14562e402521f86992a'
            '17bc8704a0e6e66b1a79ae4f58f0fdf0a58ff64db86980b2e0fa7ef352042cdb'
            '7a0fd237def865ec8376ec0f534e75ad4bc1ba6dff66774112cd2f2700ae9a9c'
            '1cc663048ef57c9238a10109bbbbef3b49e2c9d2dbd7c71c7c2f0673a6417e14'
            '912d5b8342eec3626fe61389531f7fe941c9aa6f463cf2681948a739d3c53f35'
            'b204e67c0489dfa9dba32e6f5a6ed554fca6c61d155701af35a41fea5b4db589'
            'cdd0f47ac42890fe1d8110fa1cd6106b8e143ab8e9ddb7967e891ac149380add'
            '20e3e7881960ed86b036ad64c2c4f336d72be0779ce9176bca865b7c35e41253')
noextract=(${_realname}-${pkgver}.tar.gz)

prepare() {
  [[ -d ${srcdir}/${_realname}-${pkgver} ]] && rm -rf ${srcdir}/${_realname}-${pkgver}
  tar -xzf ${srcdir}/${_realname}-${pkgver}.tar.gz -C ${srcdir} || true

  cd "${srcdir}/${_realname}-${pkgver}"

  patch -Np1 -i ${srcdir}/mikmod1.patch
  patch -Np1 -i ${srcdir}/mikmod2.patch
  patch -Np1 -i ${srcdir}/fluidsynth-volume.patch
  patch -Np1 -i ${srcdir}/double-free-crash.patch
  patch -Np1 -i ${srcdir}/SDL_mixer-find_lib.mingw.patch
  patch -Np1 -i ${srcdir}/autoreconf-fix.patch
  patch -Np1 -i ${srcdir}/non-void-return-value.patch

  #sed -e "/CONFIG_FILE_ETC/s|/etc/timidity.cfg|/etc/timidity++/timidity.cfg|" \
  #    -e "/DEFAULT_PATH/s|/etc/timidity|/etc/timidity++|" \
  #    -e "/DEFAULT_PATH2/s|/usr/local/lib/timidity|/usr/lib/timidity|" \
  #    -i timidity/config.h
  
  autoreconf -fiv
}

build() {
  [[ -d "${srcdir}/build-${MINGW_CHOST}" ]] && rm -rf "${srcdir}/build-${MINGW_CHOST}"
  mkdir -p "${srcdir}/build-${MINGW_CHOST}"
  cd "${srcdir}/build-${MINGW_CHOST}"

  export lt_cv_deplibs_check_method='pass_all'
  ../${_realname}-${pkgver}/configure \
    --prefix=${MINGW_PREFIX} \
    --build=${MINGW_CHOST} \
    --host=${MINGW_CHOST} \
    --enable-shared \
    --enable-static \
    --disable-music-mp3 \
    --enable-music-mp3-mad-gpl

  make
}

package() {
  cd "${srcdir}/build-${MINGW_CHOST}/"

  make DESTDIR="${pkgdir}/" install
  install -Dm644 "${srcdir}/${_realname}-${pkgver}/COPYING" \
    "${pkgdir}${MINGW_PREFIX}/share/licenses/${_realname}/LICENSE"
}
