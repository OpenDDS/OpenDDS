set(OPENDDS_BUILD_TESTS ${_OPENDDS_BUILD_TESTS_DEFAULT} CACHE BOOL "Build OpenDDS Tests")
set(OPENDDS_BUILD_EXAMPLES TRUE CACHE BOOL "Build OpenDDS Examples")
if(OPENDDS_BUILD_EXAMPLES OR OPENDDS_BUILD_TESTS)
  enable_testing()
  configure_file("cmake/CTestCustom.cmake" "." COPYONLY)

  add_subdirectory(DevGuideExamples/DCPS/Messenger)
  add_subdirectory(examples)
endif()
if(OPENDDS_BUILD_TESTS)
  add_subdirectory(tests)
endif()
