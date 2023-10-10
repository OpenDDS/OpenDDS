cmake_minimum_required(VERSION 3.3)
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
project(opendds_install_test_user CXX)

include(GNUInstallDirs)

set(lib opendds_install_test_library)
find_package(${lib} REQUIRED)

add_executable(${PROJECT_NAME} "user.cpp")
target_link_libraries(${PROJECT_NAME} "${lib}::${lib}")

install(TARGETS ${PROJECT_NAME}
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
