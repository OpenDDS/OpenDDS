# Using OpenDDS in a CMake Project

OpenDDS can be used with CMake-Based projects by using the OpenDDS CMake Config
package (located [here](../cmake)). This package bridges the gap between the
MPC build system used by OpenDDS and CMake-Based projects by exposing OpenDDS CMake
targets (see comments [here](../cmake/OpenDDSConfig.cmake)), and by providing the
[`OPENDDS_TARGET_SOURCES`](#adding-idl-sources-with-opendds_target_sources) macro
which simplifies IDL compilation.

## Requirements

In order to use the Config module, CMake version 3.3.2 or greater is required.
However, to run the Messenger examples, version 3.8.2 or greater is required
(due to some dependencies which simplify the custom file-copy targets).

## CMake Messenger Examples

Included with OpenDDS are two example CMake-Based Messenger applications which
exist to showcase two different strategies for adding IDL files:

  1. [Messenger with direct IDL inclusion]
  2. [Messenger with auxiliary IDL library]

The first scenario adds the IDL file directly into each executable so
the generated headers/sources are added to the publisher/subscriber targets
independently.

The second scenario creates a library which has the IDL-generated
headers/sources) added to it. The publisher and subscriber executables
then link against the library to use the IDL-generated code.

## Configure-Generated Variables

The configure script (`$DDS_ROOT/configure`) is responsible for generating the
`config.cmake` file (placed in [$DDS_ROOT/cmake](../cmake)),
which has various configuration options. These options provide the OpenDDS
CMake package with the required context it needs to integrate with the
OpenDDS code generators and libraries.

*Note:* With an installed version of OpenDDS (generated using `make install`),
`config.cmake`  will be placed in `/path/to/install-prefix-dir/lib/cmake/opendds`.  This location allows CMake to locate the file using `CMAKE_PREFIX_PATH`.

Not all of the variables in this file are used.  They are procedurally
generated from variables in the configure script. However, to get a better
sense of how they are being used take a look at the
[options.cmake](../cmake/options.cmake) file. This is where various
CMake C/C++ compile-time defines and IDL-Compiler flags are set/unset
depending upon which variables are enabled/disabled.

If you are using OpenDDS libraries that were built without the help of the
`configure` script, the `config.cmake` file needs to be created manually.

## Using the OpenDDS CMake Package

The OpenDDS CMake package is based upon the standard CMake
[packaging guidelines](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html),
so the typical generate/compile steps only require setting the `CMAKE_PREFIX_PATH` and
calling [`find_package(OpenDDS)`](https://cmake.org/cmake/help/latest/command/find_package.html)
within a CMakeLists.txt file. See the examples below for typical usage scenarios.

### Example Using OpenDDS Source Tree

To generate/compile the [Messenger with direct IDL inclusion] within the OpenDDS source tree
(assuming `source setenv.sh` on Unix or `setenv.cmd` on Windows was already run):

#### Unix

```bash
cd $DDS_ROOT/tests/cmake_integration/Messenger/Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$DDS_ROOT ..
cmake --build .
```

#### Windows

The following assumes Visual Studio 2017 using 64-bit architecture (adjust the
CMake -G parameter if using something different).

```bat
cd %DDS_ROOT%\tests\cmake_integration\Messenger\Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=%DDS_ROOT% -G "Visual Studio 15 2017 Win64" ..
cmake --build .
```

### Example Using Installed OpenDDS (Unix only)

The `--prefix` switch can be passed to configure to enable the `install` target,
which will install OpenDDS (including the OpenDDS CMake Config module) into the
specified directory.  See the [INSTALL](../INSTALL) document for details.

*Note:* Be sure to pass an absolute path to `--prefix`.

Here is an example of: configure, compile, install, and generate/compile
the Messenger_1 example from outside the source tree

```bash
DDS_WORKSPACE=$(pwd)
cd OpenDDS-src
./configure --prefix=$DDS_WORKSPACE/opendds-install --ace-github-latest
make
make install
cp -ar tests/cmake_integration/Messenger ..
cd ../Messenger/Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$DDS_WORKSPACE/opendds-install ..
cmake --build .
```
*Note:* The `--ace-github-latest` switch will download the latest ACE/TAO sources
from GitHub, as opposed to downloading the most-recently released OCITAO.

*Note:* While this will build the Messenger\_1 example, the run_test.pl script will
not work due to missing Perl dependencies.

## Adding IDL Sources with OPENDDS\_TARGET\_SOURCES

Aside from importing the various OpenDDS targets, the OpenDDS Config Package
provides an easy way to add IDL sources to CMake targets. This is achieved by
the `OPENDDS_TARGET_SOURCES` macro, which behaves similarly to the
built-in [`target_sources`](https://cmake.org/cmake/help/latest/command/target_sources.html) command except for the following:

  - Items can be either C/C++ sources or IDL sources.
  - The scope-qualifier (PUBLIC, PRIVATE, INTERFACE) is not required.
    When it is omitted, PRIVATE is used by default.
  - Command-line options can be supplied to the TAO/OpenDDS IDL compilers
    using TAO\_IDL\_OPTIONS and/or OPENDDS\_IDL\_OPTIONS (if the default
    behavior is not suitable).

When IDL sources are supplied, custom commands are generated which will
be invoked to compile the IDL sources into their component cpp/h files.

If the passed-in target is a shared library, a custom command will also be
added to generate the required IDL export header file (*target*_export.h),
which is necessary to properly export symbols for the IDL-Generated sources.

### Usage Summary

```
OPENDDS_TARGET_SOURCES(target
  [items...]
  [<INTERFACE|PUBLIC|PRIVATE> items...]
  [TAO_IDL_OPTIONS options...]
  [OPENDDS_IDL_OPTIONS options...])
```

### Example

Taken from the [Messenger with direct IDL inclusion], here is a snippet showing
how IDL files can be added directly to executable targets using the `OPENDDS_TARGET_SOURCES`
macro:

```cmake
add_executable(publisher
    ${src}/publisher.cpp
)
OPENDDS_TARGET_SOURCES(publisher
    ${src}/Writer.cpp
    ${src}/Writer.h
    ${src}/Messenger.idl
)
```

Another snippet, based upon [Messenger with auxiliary IDL lib], showcases how an
auxiliary IDL library can be created for inclusion by other executables:

```cmake
add_library(messenger)
OPENDDS_TARGET_SOURCES(messenger ${src}/Messenger.idl)

add_executable(publisher
    ${src}/publisher.cpp
    ${src}/Writer.cpp
    ${src}/Writer.h
)

target_link_libraries(publisher messenger OpenDDS::OpenDDS)

```

*Note:* This may issue a warning in earlier version of CMake due to the messenger library
not having any sources added with it in the call to `add_library`.

[Messenger with direct IDL inclusion]: ../tests/cmake_integration/Messenger/Messenger_1/CMakeLists.txt
[Messenger with auxiliary IDL lib]: ../tests/cmake_integration/Messenger/Messenger_2/CMakeLists.txt
