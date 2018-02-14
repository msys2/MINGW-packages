MINGW-packages
==============
[![TeaCI status](https://tea-ci.org/api/badges/Alexpux/MINGW-packages/status.svg)](https://tea-ci.org/Alexpux/MINGW-packages)
[![AppVeyor status](https://ci.appveyor.com/api/projects/status/github/Alexpux/mingw-packages?branch=master&svg=true)](https://ci.appveyor.com/project/Alexpux/mingw-packages)

This repository contains package scripts for MinGW-w64 targets to build under MSYS2.

MSYS2 is an independent rewrite of MSYS providing a Unix-like environment and command-line interface for Windows making it possible to port software running on POSIX systems (such as Linux, BSD, and Unix systems) to Windows.

## Documentation
If you want to read about using MINGW-packages or developing MINGW-packages yourself, you can start at the [MSYS2 wiki](https://github.com/msys2/msys2/wiki/Creating-Packages)

## Using packages
You have two options if you want to use these packages:

1. Either you can use a **pre-built** binary package from from the MSYS2 MINGW64 repo (which includes the binaries, libraries, headers, man pages), and install it on your machine, and build against those packages/libraries as you are porting/writing your software.

 Assuming you have a properly installed MSYS2 environment, you can install the pre-built binary package by using the following command from the bash prompt:
 ```
    pacman -S ${package-name}
 ```
 Please note: Not all the packages in this repository are built and accessible from the MSYS2 MINGW64 repo right away, after merging changes to the git repository it can take a few days until compiled and built packages are accessible in the repo. Also for some packages you can find older versions in the repo if you need older version, for some packages you have only the most recent version.

2. Second option is to download or clone the package folder with the scripts to your machine and you **build it for yourself**.

 Assuming you have a properly installed MSYS2 environment and build tools, you can build any package using the following command:
 ```
    cd ${package-name}
    MINGW_INSTALLS=mingw64 makepkg-mingw -sLf
 ```
 After that you can install the freshly built package(s) with the following command:
 ```
    pacman -U ${package-name}*.pkg.tar.xz
 ```
## Creating packages
 TBD

