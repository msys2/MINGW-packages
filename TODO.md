What we need to do:

* Qt Creator: Registering as a post mortem debugger doesn't work (-wincrashevent)
              reports:
              Starting executable failed:
              E:/msys64/mingw64/bin: No such file or directory.

* Post mortem debugging: Running programs from mintty prevents Windows PMD from
                         being invoked.

* Octopi: Finish this. It mostly works.

* Qt5 Static: CMake support doesn't link to ws2_32 nor use -static linkflag.

* KDE Frameworks + Konsole / Yakuake: Port all of these and replace mintty.

* GIMP: Fix loading python extensions when GIMP is starting from bash.

* Python2/3: Fix testsuite.

* Guile: Fix 64-bit building.

* Perl: Need to find maintainer for it:) It very buggy to use

* gperf: not working in many situations.

* Rust: Fix building with external Clang.

* Meld: Fix to not open itself in compare windows.

* PCRE: Fix tests failure (Julia issue).

* Fix NCURSES to work properly.

* Martell reports the following packages are without static libraries:

   fontconfig gcrypt speex theora matroska openal32 openjpeg
   theoradec theoraenc libopus libopencore-amrnb libopencore-amrwb
   libcelt0 libgnutls libgme libembl libspeexdsp

* Bison: Add path relocation when locating m4

* Add the Oyrol, Urho3D and Godot engines

* CMake Library Packages (all): Relocate find_package()-called scripts as
  was done for CGAL in https://github.com/Alexpux/MINGW-packages/commit/34ea54
