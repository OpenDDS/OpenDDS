.. news-prs: 5054

.. news-start-section: Platform Support and Dependencies
.. news-start-section: CMake
- Fixed accidental use of RapidJSON's ``CMakeLists.txt`` file when building OpenDDS with CMake.
  This caused issues such as forcing the ``CMAKE_CXX_STANDARD`` to C++11 and breaking the ACE C++ standard detection code.
- Fixed configure issue with Xerces from vcpkg when building OpenDDS with CMake.
.. news-end-section
.. news-end-section
