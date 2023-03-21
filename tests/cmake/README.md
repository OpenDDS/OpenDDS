# CMake Integration Tests

All tests in this subdirectory use CMake as a build-system and exist to test
CMake interoperability with the MPC-Generated OpenDDS codebase.

## Building and Running All the CMake Tests

If you have a new enough CMake, this can be done with:

```bash
cmake -S . -B build
cmake --build build
cd build
ctest
```

or with an older CMake:

```bash
mkdir build
cd build
cmake ..
cmake --build .
ctest
```
