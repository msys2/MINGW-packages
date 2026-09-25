# Maintainer: BlobyCZ
pkgname=mingw-w64-x86_64-dart
pkgver=0.1.3
pkgrel=1

pkgdesc='Dart and Flutter SDKs for MinGW64'
arch=('any')
mingw_arch=('x86_64')
url='https://github.com/Bloby22/mingw64_dart'
license=('MIT' 'BSD')
options=('!strip')
depends=('mingw-w64-x86_64-gcc-libs')

_commit=7e0c8cdb8c534431eacade1d69ef4b765e0f6ca5
_dart_version=3.13.4
_dart_archive=dartsdk-windows-x64-release.zip
_flutter_version=3.47.5
_flutter_archive=flutter_windows_3.47.5-stable.zip
source=(
    "https://github.com/Bloby22/mingw64_dart/archive/${_commit}.tar.gz"
    "https://storage.googleapis.com/dart-archive/channels/stable/release/${_dart_version}/sdk/${_dart_archive}"
    "https://storage.googleapis.com/flutter_infra_release/releases/stable/windows/${_flutter_archive}"
)
sha256sums=(
    '5eaac9012e9830c78a4c6d05e58b5a1a01feeb8ad327c8f7a845499a1196b0d9'
    'c38bcecee16b348694d4acc72b3781e5fa0e8766a4d0d1576182c7204ab3d763'
    '0ccd71931f49c2fbe394b1eeb6d79af3d624058a043ea0d03d34160581624fb8'
)
noextract=("${_dart_archive}" "${_flutter_archive}")

build() {
    local cxx=${CXX:-g++}
    cd "$srcdir/mingw64_dart-${_commit}"
    mkdir -p "$srcdir/build-pkg"
    "$cxx" ${CPPFLAGS} ${CXXFLAGS} -std=c++20 -municode \
        src/flutter_launcher.cpp \
        -o "$srcdir/build-pkg/flutter.exe" \
        ${LDFLAGS} \
        -lshell32

    [[ -f "$srcdir/build-pkg/flutter.exe" ]] || return 1

    rm -rf "$srcdir/build-pkg/dart-sdk"
    bsdtar -xf "$srcdir/${_dart_archive}" -C "$srcdir/build-pkg"
    chmod -R u+w "$srcdir/build-pkg/dart-sdk"
    rm -rf "$srcdir/build-pkg/flutter"
    bsdtar -xf "$srcdir/${_flutter_archive}" -C "$srcdir/build-pkg"
    chmod -R u+w "$srcdir/build-pkg/flutter"
}

package() {
    install -Dm755 "$srcdir/build-pkg/flutter.exe" "$pkgdir/mingw64/bin/flutter.exe"
    install -d "$pkgdir/mingw64/lib"
    cp -a "$srcdir/build-pkg/dart-sdk" "$pkgdir/mingw64/lib/"
    cp -a "$srcdir/build-pkg/flutter" "$pkgdir/mingw64/lib/"
    ln "$pkgdir/mingw64/lib/dart-sdk/bin/dart.exe" "$pkgdir/mingw64/bin/dart.exe"
    install -Dm644 "$srcdir/mingw64_dart-${_commit}/LICENSE-MIT" "$pkgdir/mingw64/share/licenses/$pkgname/LICENSE-MIT"
    install -Dm644 "$srcdir/mingw64_dart-${_commit}/LICENSE-BSD" "$pkgdir/mingw64/share/licenses/$pkgname/LICENSE-BSD"
}
