# Distributed under the OpenDDS License. See accompanying LICENSE
# file or http://www.opendds.org/license.html for details.

include(${CMAKE_CURRENT_LIST_DIR}/init.cmake)

set(version_file "${DDS_ROOT}/VERSION.txt")
file(READ ${version_file} version_file_contents)
string(REGEX MATCH "OpenDDS version ([0-9]+.[0-9]+.[0-9]+)" _ ${version_file_contents})
set(PACKAGE_VERSION ${CMAKE_MATCH_1})
if (NOT PACKAGE_VERSION)
  message(FATAL_ERROR "Couldn't get OpenDDS version from ${version_file}")
endif()

if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_EXACT TRUE)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  set(PACKAGE_VERSION_UNSUITABLE FALSE)

elseif(PACKAGE_FIND_VERSION VERSION_LESS PACKAGE_VERSION)
  set(PACKAGE_VERSION_EXACT FALSE)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  set(PACKAGE_VERSION_UNSUITABLE FALSE)

else()
  set(PACKAGE_VERSION_EXACT FALSE)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
  set(PACKAGE_VERSION_UNSUITABLE TRUE)
endif()
