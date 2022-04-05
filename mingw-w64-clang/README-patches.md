# Patches statuses

Legend:

- :grey_exclamation: - not meant to upstream, for compatibility with GCC only
- :x: - not upstreamed
- :grey_question: - sent but not merged yet
- :arrow_up_small:  - upstreamed
- :arrow_down_small:  - backported

-----

- `"0002-Fix-GetHostTriple-for-mingw-w64-in-msys.patch"` :x:
- `"0003-Revert-CMake-try-creating-symlink-first-on-windows.patch"` :x: (win symlinks don't play well with pacman packages)
- `"0005-add-pthread-as-system-lib-for-mingw.patch"` :grey_exclamation:
- `"0008-enable-emutls-for-mingw.patch"` :grey_exclamation:
- `"0010-lldb-Fix-building-standalone-LLDB-on-Windows.patch"` :arrow_down_small: - [https://reviews.llvm.org/D122523]
- `"0104-link-pthread-with-mingw.patch"` :grey_exclamation:
- `"0304-ignore-new-bfd-options.patch"` :x:
- `"0405-Do-not-try-to-build-CTTestTidyModule-for-Windows-with-dylibs.patch"` :arrow_down_small:
- `"0901-cast-to-make-gcc-happy.patch"` :grey_exclamation:
