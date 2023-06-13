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
- `"0005-Fix-Any-linker-error-with-multiple-compilers.patch"` :grey_question:
- `"0006-COFF-Remove-misleading-and-unclear-comments.-NFC.patch"` :arrow_down_small: https://reviews.llvm.org/D152359
- `"0007-llvm-dlltool-Clarify-parameters-simplify-ArgList-usa.patch"` :arrow_down_small: https://reviews.llvm.org/D152360
- `"0008-llvm-dlltool-Ignore-the-temp-prefix-option.patch"` :arrow_down_small: https://reviews.llvm.org/D152361
- `"0009-llvm-dlltool-Implement-the-no-leading-underscore-opt.patch"` :arrow_down_small: https://reviews.llvm.org/D152363
- `"ab8d4f5a122fde5740f8c084c8165f51a26c93c7.patch"` :arrow_down_small: https://reviews.llvm.org/D152121
- `"0101-link-pthread-with-mingw.patch"` :grey_exclamation:
- `"0102-Rename-flang-new-flang-experimental-exec-to-flang.patch"` :grey_question: https://reviews.llvm.org/D143592
- `"0303-ignore-new-bfd-options.patch"` :x:
