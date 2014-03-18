What we need to do:

* libwinpthreads/libgomp: Fix issue with semaphores.

* GIMP: Fix loading python extensions when GIMP is starting from bash.

* innoextract: Fix crashing on startup.

* Qt5: Fix building when another version is installed in system. Need rearrange 
  CFLAGS/LDFLAGS and place system wide flags at the end.

* Qt5: Consider merging PKGBUILDs of qt5 and qt5-static.

* Qt4: Add package.

* Blender: Fix to use filesystem layout as on Linux.

* Python2/3: Fix testsuite.

* Resolve conflict between QtCreator/qbs

* Guile: Fix 64-bit building.

* Perl: Need to find maintainer for it:) It very buggy to use

* gperf: not working in many situations.

* clang: 64-bit version is not properly work when using clang++

* octopi: Fix to properly work with pacman.

* ImageMagick: Fix building latest version.

* Rust: Fix building with external Clang.

* Meld: Fix to not open itself in compare windows.

* PCRE: Fix tests failure (Julia issue).

