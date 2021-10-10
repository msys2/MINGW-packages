#
# CMakeLists.txt for "lp_solve_5.5.2.0_source.tar.gz"
#
# Copyright 2012-2013 Benjamin Eikel
# Copyright 2018 yumetodo <yume-wikijp@live.jp>
# License: Public Domain
#

cmake_minimum_required(VERSION 2.8.12)

project(lpsolve C)

set(LPSOLVE_VERSION_MAJOR 5)
set(LPSOLVE_VERSION_MINOR 5)
set(LPSOLVE_VERSION_PATCH 2)
set(LPSOLVE_VERSION ${LPSOLVE_VERSION_MAJOR}.${LPSOLVE_VERSION_MINOR}.${LPSOLVE_VERSION_PATCH})
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
option(LIB_TYPE "STATIC or SHARED" SHARED)

# Set up install directories
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKECONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/lpsolve)

add_library(lpsolve55 ${LIB_TYPE}
    lp_MDO.c
    shared/commonlib.c
    shared/mmio.c
    shared/myblas.c
    ini.c
    fortify.c
    colamd/colamd.c
    lp_rlp.c
    lp_crash.c
    bfp/bfp_LUSOL/lp_LUSOL.c
    bfp/bfp_LUSOL/LUSOL/lusol.c
    lp_Hash.c
    lp_lib.c
    lp_wlp.c
    lp_matrix.c
    lp_mipbb.c
    lp_MPS.c
    lp_params.c
    lp_presolve.c
    lp_price.c
    lp_pricePSE.c
    lp_report.c
    lp_scale.c
    lp_simplex.c
    lp_SOS.c
    lp_utils.c
    yacc_read.c
)

set_property(TARGET lpsolve55 PROPERTY PUBLIC_HEADER
    lp_bit.h
    lp_crash.h
    lp_explicit.h
    lp_fortify.h
    lp_Hash.h
    lp_lib.h
    lp_matrix.h
    lp_MDO.h
    lp_mipbb.h
    lp_MPS.h
    lp_presolve.h
    lp_price.h
    lp_pricePSE.h
    lp_report.h
    lp_rlp.h
    lp_scale.h
    lp_simplex.h
    lp_solveDLL.h
    lp_SOS.h
    lp_types.h
    lp_utils.h
    lp_wlp.h
)

target_include_directories(lpsolve55 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared"
    "${CMAKE_CURRENT_SOURCE_DIR}/bfp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bfp/bfp_LUSOL"
    "${CMAKE_CURRENT_SOURCE_DIR}/bfp/bfp_LUSOL/LUSOL"
    "${CMAKE_CURRENT_SOURCE_DIR}/colamd"
)

target_compile_definitions(lpsolve55 PUBLIC
    YY_NEVER_INTERACTIVE
    PARSER_LP
    INVERSE_ACTIVE=INVERSE_LUSOL
    RoleIsExternalInvEngine
)

if(WIN32)
    target_compile_definitions(lpsolve55 PUBLIC
        INLINE=static
        _USRDLL
        WIN32
    )
endif()

include(CheckSymbolExists)
check_symbol_exists(isnan math.h HAVE_ISNAN)
if(NOT HAVE_ISNAN)
    set_property(TARGET lpsolve55 APPEND PROPERTY COMPILE_DEFINITIONS NOISNAN)
endif()

include (CheckLibraryExists)
check_library_exists(m ceil "" HAVE_LIBM)
if (HAVE_LIBM)
  target_link_libraries (lpsolve55 PUBLIC m)
endif ()

check_library_exists(dl dlopen "" HAVE_LIBDL)
if (HAVE_LIBDL)
  target_link_libraries (lpsolve55 PUBLIC dl)
endif ()

# Set version of library
set_target_properties(lpsolve55 PROPERTIES VERSION ${LPSOLVE_VERSION}
                                           SOVERSION ${LPSOLVE_VERSION_MAJOR})

# Install the library
install(TARGETS lpsolve55 EXPORT lpsolve55-export
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lpsolve
)

# Set include directories for users of this library
target_include_directories(lpsolve55 INTERFACE ${CMAKE_INSTALL_FULL_INCLUDEDIR}/lpsolve)

# Library export
install(EXPORT lpsolve55-export DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR} FILE lpsolveTargets.cmake)

# Create the configuration files
include(CMakePackageConfigHelpers)
get_property(LPSOLVE_DEFINITIONS TARGET lpsolve55 PROPERTY COMPILE_DEFINITIONS)
configure_package_config_file(lpsolveConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/lpsolveConfig.cmake
                              INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR}
                              PATH_VARS CMAKE_INSTALL_CMAKECONFIGDIR
                              NO_SET_AND_CHECK_MACRO
                              NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/lpsolveConfigVersion.cmake
                                 VERSION ${LPSOLVE_VERSION}
                                 COMPATIBILITY SameMajorVersion)

# Install the configuration files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lpsolveConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/lpsolveConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR})

# lpsolve front-end
if(${LIB_TYPE} STREQUAL "STATIC")
    add_executable (lp_solve lp_solve/lp_solve.c)
    target_include_directories(lp_solve PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries (lp_solve PUBLIC lpsolve55)
    install (TARGETS lp_solve DESTINATION bin)
endif()

# for xxx-config.cmake
install(EXPORT lpsolve55-export
    FILE lpsolve55-config.cmake
    DESTINATION share/cmake/lpsolve55/
    EXPORT_LINK_INTERFACE_LIBRARIES)
