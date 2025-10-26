#!/usr/bin/sh
# launcher script for rust-lldb

set -e

RUSTC_SYSROOT=$(${MINGW_PREFIX}/bin/rustc --print sysroot)
exec /usr/bin/sh "$RUSTC_SYSROOT/bin/rust-lldb"
