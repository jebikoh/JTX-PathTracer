#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embree" for configuration "Release"
set_property(TARGET embree APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embree PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/embree4.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/embree4.dll"
  )

list(APPEND _cmake_import_check_targets embree )
list(APPEND _cmake_import_check_files_for_embree "${_IMPORT_PREFIX}/lib/embree4.lib" "${_IMPORT_PREFIX}/bin/embree4.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
