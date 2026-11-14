cmake_minimum_required(VERSION 3.15)
project(qr-code-generator VERSION 1.8.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(qrcodegenc c/qrcodegen.c)
add_library(qrcodegencpp cpp/qrcodegen.cpp)
target_compile_features(qrcodegenc PRIVATE c_std_99)
target_compile_features(qrcodegencpp PUBLIC cxx_std_11)

target_include_directories(qrcodegenc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/c>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qrcodegenc>)
target_include_directories(qrcodegencpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qrcodegen>)

set_target_properties(qrcodegenc PROPERTIES PUBLIC_HEADER c/qrcodegen.h)
set_target_properties(qrcodegencpp PROPERTIES PUBLIC_HEADER cpp/qrcodegen.hpp)

install(TARGETS qrcodegenc EXPORT qr-code-generator-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qrcodegenc)
install(TARGETS qrcodegencpp EXPORT qr-code-generator-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qrcodegen)

configure_package_config_file(qr-code-generator-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/qr-code-generator-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qr-code-generator)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/qr-code-generator-config-version.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(EXPORT qr-code-generator-targets
    NAMESPACE qr-code-generator::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qr-code-generator)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/qr-code-generator-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/qr-code-generator-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qr-code-generator)

foreach(library_name IN ITEMS qrcodegenc qrcodegencpp)
    if(library_name STREQUAL "qrcodegenc")
        set(header_subdir qrcodegenc)
    else()
        set(header_subdir qrcodegen)
    endif()
    configure_file(qrcodegen.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/${library_name}.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${library_name}.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endforeach()
