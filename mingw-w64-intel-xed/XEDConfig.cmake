
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/XEDTargets.cmake")

set(XED_ARCHIVE              "${PACKAGE_PREFIX_DIR}/lib/libxed.dll.a")
set(XED_ILD_ARCHIVE          "${PACKAGE_PREFIX_DIR}/lib/libxed-ild.dll.a")
set(XED_BUILD_SHARED_LIBS    ON)
set(XED_INCLUDE_DIRS         "${PACKAGE_PREFIX_DIR}/include")
set(XED_INSTALL_PREFIX       ${PACKAGE_PREFIX_DIR})
set(XED_LIBRARY              "${PACKAGE_PREFIX_DIR}/bin/libxed.dll")
set(XED_ILD_LIBRARY          "${PACKAGE_PREFIX_DIR}/bin/libxed-ild.dll")
set(XED_VERSION              "@XED_VERSION@")

message(STATUS "Found XED: ${CMAKE_CURRENT_LIST_DIR}/XEDConfig.cmake (found version ${XED_VERSION})")

# XED include
include_directories("${PACKAGE_PREFIX_DIR}/include")
link_directories(BEFORE "${PACKAGE_PREFIX_DIR}/lib")
