# CMake Integration Tests

All tests in this subdirectory use CMake as a build-system and exist to test
CMake interoperability with the MPC-Generated OpenDDS codebase.

## Building and Running All the CMake Tests

```bash
mkdir build
cd build
cmake ..
cmake --build .
ctest
```
