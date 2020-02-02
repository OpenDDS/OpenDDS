# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/init.cmake)

macro(opendds_extract_version  in_version_file  out_version  out_major  out_minor)
  file(READ "${in_version_file}" contents)
  if(contents)
    string(REGEX MATCH "OpenDDS version (([0-9]+).([0-9]+))" _ "${contents}")
    set(${out_version} ${CMAKE_MATCH_1})
    set(${out_major}   ${CMAKE_MATCH_2})
    set(${out_minor}   ${CMAKE_MATCH_3})
  endif()
endmacro()

opendds_extract_version("${DDS_ROOT}/VERSION.txt"
  _OPENDDS_VERSION
  _OPENDDS_VERSION_MAJOR
  _OPENDDS_VERSION_MINOR
)

set(PACKAGE_VERSION ${_OPENDDS_VERSION})

if(PACKAGE_FIND_VERSION VERSION_EQUAL _OPENDDS_VERSION)
  set(PACKAGE_VERSION_EXACT TRUE)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  set(PACKAGE_VERSION_UNSUITABLE FALSE)

elseif(PACKAGE_FIND_VERSION VERSION_LESS _OPENDDS_VERSION)
  set(PACKAGE_VERSION_EXACT FALSE)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  set(PACKAGE_VERSION_UNSUITABLE FALSE)

else()
  set(PACKAGE_VERSION_EXACT FALSE)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
  set(PACKAGE_VERSION_UNSUITABLE TRUE)
endif()
