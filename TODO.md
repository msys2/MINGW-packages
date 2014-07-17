What we need to do:

* GIMP: Fix loading python extensions when GIMP is starting from bash.

* Qt5: Fix building when another version is installed in system. Need rearrange 
  CFLAGS/LDFLAGS and place system wide flags at the end.

* Qt5: Consider merging PKGBUILDs of qt5 and qt5-static.

* Qt4: Add package <- I (Ray) think we ignore software that hasn't moved to Qt5.
  .. it is probably not being maintained and would thus be removed soon anyway.

* Blender: Fix to use filesystem layout as on Linux.

* Python2/3: Fix testsuite.

* Guile: Fix 64-bit building.

* Perl: Need to find maintainer for it:) It very buggy to use

* gperf: not working in many situations.

* clang: 64-bit version is not properly work when using clang++

* octopi: Fix to properly work with pacman.

* Rust: Fix building with external Clang.

* Meld: Fix to not open itself in compare windows.

* PCRE: Fix tests failure (Julia issue).

* General: Compile a static bash and a static pacman into /sbin if possible.
