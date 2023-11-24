if(NOT DEFINED _OPENDDS_CMAKE_DIR)
  set(_OPENDDS_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
endif()
if(NOT "${_OPENDDS_CMAKE_DIR}" IN_LIST CMAKE_MODULE_PATH)
  list(APPEND CMAKE_MODULE_PATH "${_OPENDDS_CMAKE_DIR}")
endif()

if(NOT DEFINED OPENDDS_VERSION)
  function(_opendds_get_version var name)
    if(DEFINED "${var}")
      return()
    endif()

    foreach(dir IN LISTS ARGN)
      set(file "${dir}/VERSION.txt")
      if(EXISTS "${file}")
        break()
      endif()
    endforeach()
    if(NOT EXISTS "${file}")
      message(FATAL_ERROR "Could't find ${name} VERSION.txt file")
    endif()

    file(READ "${file}" contents)
    string(REGEX MATCH "${name} version ([0-9]+\.[0-9]+\.[0-9]+)" _ "${contents}")
    if(NOT CMAKE_MATCH_1)
      message(FATAL_ERROR "Couldn't get ${name} version from ${file}")
    endif()
    set("${var}" "${CMAKE_MATCH_1}" PARENT_SCOPE)
  endfunction()

  _opendds_get_version(OPENDDS_VERSION OpenDDS
    "${_OPENDDS_CMAKE_DIR}/../../dds"
    "${_OPENDDS_CMAKE_DIR}/.."
  )
endif()
