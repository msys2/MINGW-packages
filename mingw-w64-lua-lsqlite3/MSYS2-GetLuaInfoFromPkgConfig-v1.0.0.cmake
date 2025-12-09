# License:
#    SPDX: MIT
#
# -------
# v1.0.0:
# -------
#
#   Authors:
#      1. luau-project <luau.project@gmail.com>
# 
# --------
# Summary:
# --------
#
# CMake script to aid in the
# process to build, test
# and install Lua modules
# on MSYS2, out of LuaRocks.
#
# ------------
# Description:
# ------------
# 
#   This CMake script gets
#   Lua paths and info from
#   the pkg-config file (.pc)
#   for the required Lua version.
#
#   As a result, this scripts sets
#   a few variables to aid in the
#   process to build, test
#   and install Lua modules
#   out of LuaRocks.
#
# ------
# Notes:
# ------
#
#   1. CMake 3.20 or newer is required;
#
#   2. When the variable/parameter
#      IS_CURRENT_VERSION is set to ON,
#      this script assumes to handle
#      the latest Lua version available on
#      mingw-w64-lua package. Otherwise,
#      the variable/parameter LUA_VERSION
#      is assumed to be X.Y such that
#      X is the major Lua version, and
#      Y is the minor.
#
# ----------------------------
# List of CMake variables set:
# ----------------------------
#
#   * Lua_V: Lua version in the X.Y format
#   * Lua_R: Lua version in the X.Y.Z format
#
#   * Lua_prefix: absolute PATH to Lua prefix
#   * Lua_INSTALL_BIN: absolute PATH to Lua binary directory
#   * Lua_INSTALL_INC: absolute PATH to Lua include directory
#   * Lua_INSTALL_LIB: absolute PATH to Lua library directory
#   * Lua_INSTALL_MAN: absolute PATH to Lua man directory
#   * Lua_INSTALL_LMOD: absolute PATH to Lua modules (.lua files) directory
#   * Lua_INSTALL_CMOD: absolute PATH to Lua C modules directory
#   * Lua_exec_prefix: absolute PATH to Lua exec_prefix
#   * Lua_libdir: absolute PATH to Lua libdir
#   * Lua_includedir: absolute PATH to Lua includedir
#
#   * RELATIVE_Lua_prefix: relative PATH from Lua_prefix to Lua_prefix
#   * RELATIVE_Lua_INSTALL_BIN: relative PATH from Lua_prefix to Lua_INSTALL_BIN
#   * RELATIVE_Lua_INSTALL_INC: relative PATH from Lua_prefix to Lua_INSTALL_INC
#   * RELATIVE_Lua_INSTALL_LIB: relative PATH from Lua_prefix to Lua_INSTALL_LIB
#   * RELATIVE_Lua_INSTALL_MAN: relative PATH from Lua_prefix to Lua_INSTALL_MAN
#   * RELATIVE_Lua_INSTALL_LMOD: relative PATH from Lua_prefix to Lua_INSTALL_LMOD
#   * RELATIVE_Lua_INSTALL_CMOD: relative PATH from Lua_prefix to Lua_INSTALL_CMOD
#   * RELATIVE_Lua_exec_prefix: relative PATH from Lua_prefix to Lua_exec_prefix
#   * RELATIVE_Lua_libdir: relative PATH from Lua_prefix to Lua_libdir
#   * RELATIVE_Lua_includedir: relative PATH from Lua_prefix to Lua_includedir
#
#   * Lua_EXE: absolute PATH to the Lua interpreter
#   * Lua_DLL: absolute PATH to the Lua DLL
#

if (IS_CURRENT_VERSION)
    set(_LUA_PKG_CONFIG_SUFFIX "")
else()
    if (NOT DEFINED LUA_VERSION)
        message(FATAL_ERROR "Missing LUA_VERSION.")
    endif()

    if (NOT ( LUA_VERSION MATCHES [[^[0-9]+\.[0-9]+$]] ))
        message(FATAL_ERROR "Invalid LUA_VERSION format.")
    endif()

    string(REPLACE "\." "" _LUA_VERSION_SHORT ${LUA_VERSION})
    set(_LUA_PKG_CONFIG_SUFFIX ${LUA_VERSION})
endif()

set(_LUA_PKG_CONFIG_MODULE_NAME "lua${_LUA_PKG_CONFIG_SUFFIX}")

include(FindPkgConfig)

if (NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config not found")
endif()

pkg_check_modules(_Lua ${_LUA_PKG_CONFIG_MODULE_NAME})

if (NOT _Lua_FOUND)
    message(FATAL_ERROR "Lua not found")
endif()

set(_lua_pkg_config_dirs
    "prefix" "INSTALL_BIN" "INSTALL_INC"
    "INSTALL_LIB" "INSTALL_MAN" "INSTALL_LMOD"
    "INSTALL_CMOD" "exec_prefix" "libdir"
    "includedir")

set(_lua_pkg_config_vars
    "V" "R" ${_lua_pkg_config_dirs})

foreach(_lua_pkg_config_var ${_lua_pkg_config_vars})
    pkg_get_variable(Lua_${_lua_pkg_config_var} ${_LUA_PKG_CONFIG_MODULE_NAME} ${_lua_pkg_config_var})

    if (Lua_${_lua_pkg_config_var} STREQUAL "")
        message(FATAL_ERROR "pkg-config variable \"${_lua_pkg_config_var}\" not found on ${_LUA_PKG_CONFIG_MODULE_NAME}.pc")
    endif()

    if (_lua_pkg_config_var IN_LIST _lua_pkg_config_dirs)
        cmake_path(RELATIVE_PATH Lua_${_lua_pkg_config_var}
            BASE_DIRECTORY ${Lua_prefix}
            OUTPUT_VARIABLE RELATIVE_Lua_${_lua_pkg_config_var})
    endif()
endforeach()

if (NOT DEFINED _LUA_VERSION_SHORT)
	string(REPLACE "\." "" _LUA_VERSION_SHORT ${Lua_V})
endif()

find_program(Lua_EXE
    NAME "lua${_LUA_PKG_CONFIG_SUFFIX}.exe"
    HINTS ${Lua_INSTALL_BIN}
    NO_DEFAULT_PATH
    REQUIRED
)

find_file(Lua_DLL
    NAME "lua${_LUA_VERSION_SHORT}.dll"
    HINTS ${Lua_INSTALL_BIN}
    NO_DEFAULT_PATH
    REQUIRED
)