# OpenSSL 3 compatibility package

This package retains OpenSSL 3.6.5 for binaries that still require the OpenSSL 3
DLLs while the default `openssl` package transitions to OpenSSL 4.

The versioned runtime DLLs remain in `${MINGW_PREFIX}/bin`. Development headers
are in `${MINGW_PREFIX}/include/openssl-3/openssl`, and libraries, engines,
provider modules, pkg-config files, and CMake package files are under
`${MINGW_PREFIX}/lib/openssl-3`.

No command-line tools, documentation, or configuration files are installed.
The libraries use the existing `${MINGW_PREFIX}/etc/ssl` configuration and
certificate directory. There are no conflicts, replacements, or `openssl`
provides: this package can be installed alongside the default OpenSSL package.

Normal builds continue to discover OpenSSL 4. To opt into OpenSSL 3, set
`PKG_CONFIG_PATH=${MINGW_PREFIX}/lib/openssl-3/pkgconfig` for that build, or set
`OpenSSL_DIR=${MINGW_PREFIX}/lib/openssl-3/cmake/OpenSSL` when using CMake's
config package mode.

For Rust's `openssl-sys` crate, explicit paths can be supplied with
`OPENSSL_INCLUDE_DIR=${MINGW_PREFIX}/include/openssl-3` and
`OPENSSL_LIB_DIR=${MINGW_PREFIX}/lib/openssl-3`. These overrides are not needed
when rebuilding consumers for OpenSSL 4; simply installing the compatibility
DLLs keeps the old build tools runnable.
