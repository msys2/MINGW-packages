# Patches statuses

Legend:

- :grey_exclamation: - not meant to upstream, for compatibility with GCC only
- :x: - not upstreamed
- :grey_question: - sent but not merged yet
- :arrow_up_small:  - upstreamed
- :arrow_down_small:  - backported

-----

- `"0001-Fix-GetHostTriple-for-mingw-w64-in-msys.patch"` :x:
- `"0002-Revert-CMake-try-creating-symlink-first-on-windows.patch"` :x: (win symlinks don't play well with pacman packages)
- `"0003-add-pthread-as-system-lib-for-mingw.patch"` :grey_exclamation:
- `"0004-enable-emutls-for-mingw.patch"` :grey_exclamation:
- `"0005-export-out-of-tree-mlir-targets.patch"` :x:
- `"0006-lldb-Fix-building-standalone-LLDB-on-Windows.patch"` :arrow_down_small: - [https://reviews.llvm.org/D122523]
- `"0007-MinGW-Don-t-currently-set-visibility-hidden-when-bui.patch"` :arrow_down_small:
- `"0008-COFF-Emit-embedded-exclude-symbols-directives-for-hi.patch"` :arrow_down_small:
- `"0009-Use-hidden-visibility-when-building-for-MinGW-with-Clang.patch"` :arrow_down_small:
- `"0101-link-pthread-with-mingw.patch"` :grey_exclamation:
- `"0301-LLD-COFF-Add-support-for-a-new-mingw-specific-embedd.patch"` :arrow_down_small:
- `"0302-LLD-MinGW-Implement-the-exclude-symbols-option.patch"` :arrow_down_small:
- `"0303-ignore-new-bfd-options.patch"` :x:
- `"0405-Do-not-try-to-build-CTTestTidyModule-for-Windows-with-dylibs.patch"` :arrow_down_small:
