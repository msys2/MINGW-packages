# Patches statuses

Legend:

- :grey_exclamation: - not meant to upstream, for compatibility with GCC only
- :x: - not upstreamed
- :grey_question: - sent but not merged yet
- :arrow_up_small:  - upstreamed
- :arrow_down_small:  - backported

-----

- `"0001-Use-posix-style-path-separators-with-MinGW.patch"` :x::x::x: - this one is really imporant
- `"0002-Fix-GetHostTriple-for-mingw-w64-in-msys.patch"` :x:
- `"0003-CMake-try-creating-symlink-first-on-windows.patch"` :x: (win symlinks don't play well with pacman packages)
- `"0005-add-pthread-as-system-lib-for-mingw.patch"` :grey_exclamation:
- `"0008-enable-emutls-for-mingw.patch"` :grey_exclamation:
- `"0010-COFF-Move-section-name-encoding-into-BinaryFormat.patch"` :arrow_down_small: - [https://reviews.llvm.org/D118793]
- `"0011-llvm-objcopy-COFF-Fix-section-name-encoding.patch"` :arrow_down_small: - [https://reviews.llvm.org/D118692]
- `"0101-Disable-fPIC-errors.patch"` :x:
- `"0103-Use-posix-style-path-separators-with-MinGW.patch"` :x::x::x: - this one is really imporant
- `"0104-link-pthread-with-mingw.patch"` :grey_exclamation:
- `"0105-clang-Tooling-Use-Windows-command-lines.patch"` :arrow_up_small: - [https://reviews.llvm.org/D111195]
- `"0304-ignore-new-bfd-options.patch"` :x:
- `"0901-cast-to-make-gcc-happy.patch"` :grey_exclamation:
