# Messenger Example

A very basic OpenDDS example, covered in the OpenDDS Developer's Guide.

If you just want to build and run this, it will be built along with OpenDDS and
can be run using `perl run_test.pl` assuming the `setenv` file has been sourced
(see [INSTALL.md](../../../INSTALL.md) for details). To have it use the DDS
standard RTPS discovery and transport instead of the OpenDDS specific InfoRepo
discovery and TCP transports, run `perl run_test.pl --rtps` instead.

## CMake

There is a `CMakeLists.txt` provided that can be used to build this example.
Follow these steps to do that:
 - Make sure your environment is set correctly.
 - Make a `build` directory in this directory and `cd` to it to do an
   "out-of-source" build.
 - Run `cmake ..` to generate the build and `cmake --build .` to build.
 - Then it can be run the same way as described above, but in the `build`
   directory.
