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
- `"0101-link-pthread-with-mingw.patch"` :grey_exclamation:
- `"0301-Add-exceptions-for-Flang-runtime-libraries-on-MinGW.patch"` :upstreamed:
- `"0303-ignore-new-bfd-options.patch"` :x:
