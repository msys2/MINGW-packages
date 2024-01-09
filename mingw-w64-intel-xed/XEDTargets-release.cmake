#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "XED::XED" for configuration "Release"
set_property(TARGET XED::XED APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(XED::XED PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libxed.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libxed.dll"
  )

set_property(TARGET XED::ILD APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(XED::ILD PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libxed-ild.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libxed-ild.dll"
  )
  
list(APPEND _cmake_import_check_targets XED::XED )
list(APPEND _cmake_import_check_files_for_XED::XED "${_IMPORT_PREFIX}/lib/libxed.dll.a" "${_IMPORT_PREFIX}/bin/libxed.dll" )

list(APPEND _cmake_import_check_targets XED::ILD )
list(APPEND _cmake_import_check_files_for_XED::ILD "${_IMPORT_PREFIX}/lib/libxed-ild.dll.a" "${_IMPORT_PREFIX}/bin/libxed-ild.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
