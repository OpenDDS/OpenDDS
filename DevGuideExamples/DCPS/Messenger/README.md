# Messenger Example

A very basic OpenDDS example, covered in the OpenDDS Developer's Guide.

If you just want to build and run this, it will be built along with OpenDDS and
can be run using `perl run_test.pl` assuming the `setenv` file has been sourced
(see [INSTALL.md](../../../INSTALL.md) for details). To have it use the
OpenDDS-specific InfoRepo discovery instead of the DDS standard RTPS discovery,
run `perl run_test.pl --info-repo` instead.

## CMake

There is an example `CMakeLists.txt` provided, but it can conflict with the
default MPC-based build system. If you want to build this example with CMake,
these are the recommend steps:

 - Make sure your environment is set correctly.
 - Clean all the existing build files out of this directory. If this is a git
   repository you can use `git clean -dfX .` in this directory to do this.
   Otherwise at least remove all the `Messenger*.h` files, because these will
   conflict even if you do an "out-of-source" build with CMake.
 - Next make a `build` directory in this directory and `cd` to it to do an
   "out-of-source" build.
 - Run `cmake ..` to generate the build and `cmake --build .` to build.
 - To run the example you can copy `run_test.pl` to the `build` directory and
   use that or run `./publisher` and `./subscriber` at the same time.
