<p align="center">
  <a title="msys2.github.io" href="https://msys2.github.io"><img src="https://img.shields.io/website.svg?label=msys2.github.io&longCache=true&style=flat-square&url=http%3A%2F%2Fmsys2.github.io%2Findex.html&logo=github"></a><!--
  -->
  <a title="Join the chat at https://gitter.im/msys2/msys2" href="https://gitter.im/msys2/msys2"><img src="https://img.shields.io/badge/chat-on%20gitter-4db797.svg?longCache=true&style=flat-square&logo=gitter&logoColor=e8ecef"></a><!--
  -->
  <a title="GitHub Actions" href="https://github.com/msys2/MINGW-packages/actions?query=workflow%3Amain"><img alt="'main' workflow Status" src="https://img.shields.io/github/workflow/status/msys2/MINGW-packages/main?longCache=true&style=flat-square&label=build&logo=github"></a><!--
  -->
  <a title="AppVeyor" href="https://ci.appveyor.com/project/Alexpux/mingw-packages"><img src="https://img.shields.io/appveyor/ci/Alexpux/mingw-packages/master.svg?logo=appveyor&logoColor=e8ecef&style=flat-square"></a><!--
  -->
  <a title="Azure DevOps" href="https://dev.azure.com/msys2/mingw/_build/latest?definitionId=4&branchName=master"><img src="https://img.shields.io/azure-devops/build/msys2/5ee43462-f2c2-45d5-8c1c-31fdb1fd15b4/4/master?style=flat-square&logo=azure-pipelines"></a><!--
  -->
</p>

# MINGW-packages

This repository contains package scripts for MinGW-w64 targets to build under MSYS2.

MSYS2 is an independent rewrite of MSYS providing a Unix-like environment and command-line interface for Windows making it possible to port software running on POSIX systems (such as Linux, BSD, and Unix systems) to Windows.

## Documentation
See the [MSYS2 wiki](https://github.com/msys2/msys2/wiki).

## Using packages
You have two options if you want to use these packages:

1. Either you can use a **pre-built** binary package from the MSYS2 MINGW64 repo (which includes the binaries, libraries, headers, man pages), and install it on your machine, and build against those packages/libraries as you are porting/writing your software.

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
See the [MSYS2 wiki](https://github.com/msys2/msys2/wiki/Creating-Packages) for instructions and advice about creating MINGW-packages.

## License

MSYS2-packages is licensed under BSD 3-Clause "New" or "Revised" License.
A full copy of the license is provided in [LICENSE](LICENSE).
