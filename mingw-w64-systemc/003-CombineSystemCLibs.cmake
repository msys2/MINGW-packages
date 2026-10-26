# Helper script to combine SystemC DLL import lib with the main static lib
# on MinGW (non-MSVC) Windows builds.
#
# The upstream approach passes globs like "*.o *.obj" to ar through cmd.exe,
# which doesn't expand wildcards for external commands, causing:
#   ar: *.o: Invalid argument
#
# Strategy: the small static lib (systemc) contains only sc_main objects.
# The import lib (systemc-VERSION.dll.a) contains the DLL exports.
# We just need to add the sc_main objects to the import lib.
#
# Usage:
#   cmake -DCOMBINE_LIB=<path> -DIMPORT_LIB=<path>
#         -DCMAKE_AR=<path-to-ar> -DCMAKE_RANLIB=<path-to-ranlib>
#         -P CombineSystemCLibs.cmake

message(STATUS "Combining SystemC libs...")

# CMAKE_AR / CMAKE_RANLIB are not available in -P mode (no project()),
# so they must be passed as -D variables.
if(NOT CMAKE_AR OR NOT CMAKE_RANLIB)
  message(FATAL_ERROR "CMAKE_AR and CMAKE_RANLIB must be passed as -D arguments")
endif()

# Verify input files exist
if(NOT EXISTS "${COMBINE_LIB}")
  message(FATAL_ERROR "COMBINE_LIB not found: ${COMBINE_LIB}")
endif()
if(NOT EXISTS "${IMPORT_LIB}")
  message(FATAL_ERROR "IMPORT_LIB not found: ${IMPORT_LIB}")
endif()

# Extract the few objects from the small static lib only
execute_process(COMMAND "${CMAKE_AR}" x "${COMBINE_LIB}" RESULT_VARIABLE _r)
if(NOT _r EQUAL 0)
  message(FATAL_ERROR "ar x ${COMBINE_LIB} failed: ${_r}")
endif()

file(GLOB_RECURSE _small_objs "*.o" "*.obj")
list(LENGTH _small_objs _n)
message(STATUS "Extracted ${_n} objects from ${COMBINE_LIB}")

# Replace the small static lib with the import lib
file(REMOVE "${COMBINE_LIB}")
file(RENAME "${IMPORT_LIB}" "${COMBINE_LIB}")

# Add the extracted sc_main objects to the combined lib
foreach(_obj IN LISTS _small_objs)
  execute_process(COMMAND "${CMAKE_AR}" q "${COMBINE_LIB}" "${_obj}"
                  RESULT_VARIABLE _r)
  if(NOT _r EQUAL 0)
    message(FATAL_ERROR "ar q ${COMBINE_LIB} ${_obj} failed: ${_r}")
  endif()
endforeach()

# Re-index
execute_process(COMMAND "${CMAKE_RANLIB}" "${COMBINE_LIB}" RESULT_VARIABLE _r)
if(NOT _r EQUAL 0)
  message(FATAL_ERROR "ranlib ${COMBINE_LIB} failed: ${_r}")
endif()

# Clean up extracted objects
file(REMOVE ${_small_objs})

message(STATUS "SystemC libs combined successfully")
