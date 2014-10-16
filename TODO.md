What we need to do:

* GIMP: Fix loading python extensions when GIMP is starting from bash.

* Python2/3: Fix testsuite.

* Guile: Fix 64-bit building.

* Perl: Need to find maintainer for it:) It very buggy to use

* gperf: not working in many situations.

* octopi: Fix to properly work with pacman.

* Rust: Fix building with external Clang.

* Meld: Fix to not open itself in compare windows.

* PCRE: Fix tests failure (Julia issue).

* Fix NCURSES to work properly.

* General: Compile a static bash and a static pacman into /sbin if possible.

* Martell reports the following packages are without static libraries:

   fontconfig gcrypt speex theora matroska openal32 openjpeg
   theoradec theoraenc libopus libopencore-amrnb libopencore-amrwb
   libcelt0 libgnutls libgme libembl libspeexdsp
