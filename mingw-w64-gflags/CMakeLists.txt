cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# ----------------------------------------------------------------------------
# Project Setup
project(gflags VERSION 2.2.2 LANGUAGES CXX)

set(PACKAGE_NAME        "gflags")
set(PACKAGE_BUGREPORT   "https://github.com/gflags/gflags/issues")
set(PACKAGE_DESCRIPTION "A commandline flags library that allows for distributed flags.")
set(PACKAGE_URL         "http://gflags.github.io/gflags")

# ----------------------------------------------------------------------------
# System Checks (Preserved from Original)
include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake")
include(CheckIncludeFileCXX)
include(CheckCXXSymbolExists)
include(CheckTypeSize)
include(CheckLibraryExists)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ----------------------------------------------------------------------------
# Library Configuration Options (Modified for Dual Build)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_WITH_THREADS "Build multi-threaded library" ON)

set (GFLAGS_INCLUDE_DIR "${PACKAGE_NAME}")
# ----------------------------------------------------------------------------
# Configuration Variables (Preserved)
set(PUBLIC_HDRS
  "gflags.h"
  "gflags_declare.h"
  "gflags_completions.h"
)

set(PRIVATE_HDRS
  "defines.h"
  "config.h"
  "util.h"
  "mutex.h"
)

set(GFLAGS_SRCS
  "gflags.cc"
  "gflags_reporting.cc"
  "gflags_completions.cc"
)

if(WIN32 AND NOT CYGWIN)
  list(APPEND PRIVATE_HDRS "windows_port.h")
  list(APPEND GFLAGS_SRCS "windows_port.cc")
  set(OS_WINDOWS 1)
else()
  set(OS_WINDOWS 0)
endif()

# ----------------------------------------------------------------------------
# Namespace Handling (Preserved)
set(GFLAGS_NAMESPACE "gflags" CACHE STRING "Primary namespace")
set(GFLAGS_NAMESPACE_SECONDARY "" CACHE STRING "Secondary namespaces")

if(GFLAGS_NAMESPACE_SECONDARY)
  foreach(ns IN LISTS GFLAGS_NAMESPACE_SECONDARY)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/src/gflags_ns.h.in"
      "${CMAKE_CURRENT_BINARY_DIR}/include/gflags/gflags_${ns}.h"
    )
    list(APPEND PUBLIC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/include/gflags/gflags_${ns}.h")
  endforeach()
endif()

# ----------------------------------------------------------------------------
# System Configuration (Preserved)
if(MSVC)
  set(HAVE_SYS_TYPES_H 1)
  set(HAVE_STDDEF_H 1)
  set(HAVE_UNISTD_H 0)
  set(HAVE_SYS_STAT_H 1)
  set(HAVE_SHLWAPI_H 1)
  if(MSVC_VERSION VERSION_LESS 1600)
    check_include_file_cxx ("stdint.h" HAVE_STDINT_H)
    bool_to_int (HAVE_STDINT_H)  # used in #if directive
  else()
    set(HAVE_STDINT_H 1)
  endif()
  if (MSVC_VERSION VERSION_LESS 1800)
    check_include_file_cxx ("inttypes.h" HAVE_INTTYPES_H)
    bool_to_int (HAVE_INTTYPES_H)  # used in #if directive
  else()
    set(HAVE_INTTYPES_H 1)
  endif()
else()
  foreach (fname IN ITEMS unistd stdint inttypes sys/types sys/stat fnmatch)
    string(TOUPPER "${fname}" FNAME)
    string(REPLACE "/" "_" FNAME "${FNAME}")
    if (NOT HAVE_${FNAME}_H)
      check_include_file_cxx ("${fname}.h" HAVE_${FNAME}_H)
    endif()
  endforeach ()
  if (NOT HAVE_FNMATCH_H AND OS_WINDOWS)
    check_include_file_cxx ("shlwapi.h" HAVE_SHLWAPI_H)
  endif()
  # the following are used in #if directives not #ifdef
  bool_to_int (HAVE_STDINT_H)
  bool_to_int (HAVE_SYS_TYPES_H)
  bool_to_int (HAVE_INTTYPES_H)
endif()

gflags_define (STRING INTTYPES_FORMAT "Format of integer types: \"C99\" (uint32_t), \"BSD\" (u_int32_t), \"VC7\" (__int32)" "")
gflags_property (INTTYPES_FORMAT STRINGS "C99;BSD;VC7")
gflags_property (INTTYPES_FORMAT ADVANCED TRUE)
if (NOT INTTYPES_FORMAT)
  set (TYPES uint32_t u_int32_t)
  if (MSVC)
    list (INSERT TYPES 0 __int32)
  endif()
  foreach (type IN LISTS TYPES)
    check_type_size (${type} ${type} LANGUAGE CXX)
    if (HAVE_${type})
      break ()
    endif()
  endforeach ()
  if (HAVE_uint32_t)
    gflags_set (INTTYPES_FORMAT C99)
  elseif (HAVE_u_int32_t)
    gflags_set (INTTYPES_FORMAT BSD)
  elseif (HAVE___int32)
    gflags_set (INTTYPES_FORMAT VC7)
  else()
    gflags_property (INTTYPES_FORMAT ADVANCED FALSE)
    message (FATAL_ERROR "Do not know how to define a 32-bit integer quantity on your system!"
                         " Neither uint32_t, u_int32_t, nor __int32 seem to be available."
                         " Set [GFLAGS_]INTTYPES_FORMAT to either C99, BSD, or VC7 and try again.")
  endif()
endif()
# use of special characters in strings to circumvent bug #0008226
if ("^${INTTYPES_FORMAT}$" STREQUAL "^WIN$")
  gflags_set (INTTYPES_FORMAT VC7)
endif()
if (NOT INTTYPES_FORMAT MATCHES "^(C99|BSD|VC7)$")
  message (FATAL_ERROR "Invalid value for [GFLAGS_]INTTYPES_FORMAT! Choose one of \"C99\", \"BSD\", or \"VC7\"")
endif()
set(GFLAGS_INTTYPES_FORMAT "${INTTYPES_FORMAT}")
set(GFLAGS_INTTYPES_FORMAT_C99 0)
set(GFLAGS_INTTYPES_FORMAT_BSD 0)
set(GFLAGS_INTTYPES_FORMAT_VC7 0)
set("GFLAGS_INTTYPES_FORMAT_${INTTYPES_FORMAT}" 1)

if (MSVC)
  set (HAVE_strtoll 0)
  set (HAVE_strtoq  0)
else()
  check_cxx_symbol_exists (strtoll stdlib.h HAVE_STRTOLL)
  if (NOT HAVE_STRTOLL)
    check_cxx_symbol_exists (strtoq stdlib.h HAVE_STRTOQ)
  endif()
endif()

if (BUILD_WITH_THREADS)
  set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
  find_package (Threads)
  if (Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
    set (HAVE_PTHREAD 1)
    check_type_size (pthread_rwlock_t RWLOCK LANGUAGE CXX)
  else()
    set (HAVE_PTHREAD 0)
  endif()
  if (UNIX AND NOT HAVE_PTHREAD)
    if (CMAKE_HAVE_PTHREAD_H)
      set(what "library")
    else ()
      set(what ".h file")
    endif()
    message (FATAL_ERROR "Could not find pthread${what}. Check the log file"
                         "\n\t${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log"
                         "\nor disable the build of the multi-threaded gflags library (BUILD_WITH_THREADS=OFF).")
  endif()
else()
  set(HAVE_PTHREAD 0)
endif()

configure_headers (PUBLIC_HDRS  ${PUBLIC_HDRS})
configure_sources (PRIVATE_HDRS ${PRIVATE_HDRS})
configure_sources (GFLAGS_SRCS  ${GFLAGS_SRCS})

# ----------------------------------------------------------------------------
# Target Creation
if(BUILD_SHARED_LIBS)
  set (type_suffix "_shared")
  set (LIB_TYPE SHARED)
  set (GFLAGS_IS_A_DLL 1)
else()
  set (type_suffix "_static")
  set (LIB_TYPE STATIC)
  set (GFLAGS_IS_A_DLL 0)
endif()

set (BUILD_gflags_LIB ${BUILD_WITH_THREADS})
if(BUILD_WITH_THREADS)
  set (thread_suffix "")
else()
  set (thread_suffix "_nothreads")
endif()

set (target_name "gflags${thread_suffix}")

add_library(gflags ${LIB_TYPE} ${GFLAGS_SRCS} ${PRIVATE_HDRS} ${PUBLIC_HDRS})
add_library(gflags::gflags ALIAS gflags)

set_target_properties(gflags PROPERTIES
  OUTPUT_NAME "${target_name}"
  VERSION     "${PACKAGE_VERSION}"
  SOVERSION   "${PACKAGE_SOVERSION}"
)

set (include_dirs "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>")
list (APPEND include_dirs "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>")

target_include_directories (gflags
  PUBLIC  "${include_dirs}"
  PRIVATE "${PROJECT_SOURCE_DIR}/src;${PROJECT_BINARY_DIR}/include/${GFLAGS_INCLUDE_DIR}"
)

# Compile Definitions (Modified with proper GFLAGS_IS_A_DLL handling)
target_compile_definitions(gflags PUBLIC GFLAGS_IS_A_DLL=${GFLAGS_IS_A_DLL})

if (NOT BUILD_WITH_THREADS)
  target_compile_definitions (gflags PRIVATE NO_THREADS)
elseif (CMAKE_USE_PTHREADS_INIT)
  target_link_libraries (gflags ${CMAKE_THREAD_LIBS_INIT})
endif()

if (HAVE_SHLWAPI_H)
  target_link_libraries (gflags shlwapi)
endif ()


if (NOT DEFINED GFLAGS_INSTALL_CMAKEDIR)
   set(GFLAGS_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/gflags"
       CACHE STRING "Path to gflags CMake files")
endif ()

install(TARGETS gflags
        EXPORT gflags_Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development)

install (FILES ${PUBLIC_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${GFLAGS_INCLUDE_DIR})

install(EXPORT gflags_Targets
        DESTINATION ${GFLAGS_INSTALL_CMAKEDIR} COMPONENT Development
        NAMESPACE gflags::
        FILE gflags${thread_suffix}${type_suffix}-targets.cmake)

if (UNIX)
  install (PROGRAMS src/gflags_completions.sh DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

write_basic_package_version_file(
    gflagsConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/gflagsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/gflagsConfigVersion.cmake"
        DESTINATION ${GFLAGS_INSTALL_CMAKEDIR} COMPONENT Development)

set (PKGCONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
configure_file ("cmake/package.pc.in" "${PROJECT_BINARY_DIR}/${PACKAGE_NAME}.pc" @ONLY)
install (FILES "${PROJECT_BINARY_DIR}/${PACKAGE_NAME}.pc" DESTINATION "${PKGCONFIG_INSTALL_DIR}")

# ----------------------------------------------------------------------------
# Testing (Preserved)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
