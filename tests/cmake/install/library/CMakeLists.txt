project(opendds_install_library_test CXX)
cmake_minimum_required(VERSION 3.8.2)

include(GNUInstallDirs)

find_package(OpenDDS REQUIRED)

set(include_dest "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
add_library(${PROJECT_NAME} SHARED "install_library.cpp")
target_include_directories(${PROJECT_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${include_dest}>"
)

target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Dcps)
OPENDDS_TARGET_SOURCES(${PROJECT_NAME}
  PUBLIC
    "include/MovingObject.idl"
    "include/Vector.idl"
  PRIVATE
    "include/secret.idl"
  OPENDDS_IDL_OPTIONS -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

include(GNUInstallDirs)
export(TARGETS ${PROJECT_NAME} FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(targets "${PROJECT_NAME}Targets")
install(TARGETS ${PROJECT_NAME}
  EXPORT ${targets}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${include_dest}
)
set(cmake_dest "${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME}")
install(EXPORT ${targets}
  FILE "${targets}.cmake"
  NAMESPACE "${PROJECT_NAME}::"
  DESTINATION ${cmake_dest}
)
install(DIRECTORY "include/" DESTINATION ${include_dest}
  FILES_MATCHING PATTERN "*.h")
get_target_property(passed_files ${PROJECT_NAME} OPENDDS_ALL_PASSED_INTERFACE_FILES)
foreach(file ${passed_files})
  file(RELATIVE_PATH dest ${CMAKE_CURRENT_SOURCE_DIR} ${file})
  string(REGEX REPLACE "^include/" "" dest ${dest})
  get_filename_component(dest ${dest} DIRECTORY)
  install(FILES ${file} DESTINATION "${include_dest}/${dest}")
endforeach()
get_target_property(generated_files ${PROJECT_NAME} OPENDDS_ALL_GENERATED_INTERFACE_FILES)
get_target_property(generated_directory ${PROJECT_NAME} OPENDDS_GENERATED_DIRECTORY)
foreach(file ${generated_files})
  file(RELATIVE_PATH dest ${generated_directory} ${file})
  string(REGEX REPLACE "^include/" "" dest ${dest})
  get_filename_component(dest ${dest} DIRECTORY)
  install(FILES ${file} DESTINATION "${include_dest}/${dest}")
endforeach()
