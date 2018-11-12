
# Integrating OpenDDS with CMake

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
exist to showcase two different strategies for adding IDL files (internally they
also test different aspects of the IDL-Compilation mechanism):

  1. [Messenger with direct IDL inclusion](../tests/cmake_integration/Messenger/Messenger_1/CMakeLists.txt)
  2. [Messenger with auxiliary IDL lib](../tests/cmake_integration/Messenger/Messenger_2/CMakeLists.txt)

The first scenario creates an intermediary library which has the IDL file (and
the generated headers/sources) added to it. The publisher/subscriber in this case
then link against the library to interact with the IDL-Generated code.

The second scenario simply adds the IDL file directly into each executable so
the generated headers/sources are added to the publisher/subscriber targets
independently.

## Configure-Generated output

The configure script ($DDS_ROOT/configure) is responsible for generating the
config.cmake file (placed in [$DDS_ROOT/cmake](../cmake)),
which houses various configuration options. These options provide the OpenDDS
CMake package with the required context it needs to integrate with the
collection of MPC-Generated OpenDDS libraries.

*Note:* With an installed version of OpenDDS (generated using `make install`) the
`config.cmake` file will be placed within `/path/to/install-prefix-dir/lib/cmake/opendds`.

Not all of the variables in this file are used as they are procedurally
generated from variables in the configure script. However, to get a better
sense of how they are being used take a look at the
[options.cmake](../cmake/options.cmake) file. This is where various
CMake C/C++ compile-time defines and IDL-Compiler flags are set/unset
depending upon which variables are enabled/disabled.

## Using the OpenDDS CMake Package

The OpenDDS CMake package is based upon the standard CMake
[packaging guidelines](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html),
so the typical generate/compile steps only require setting the `CMAKE_PREFIX_PATH` and
calling [`find_package(OpenDDS)`](https://cmake.org/cmake/help/latest/command/find_package.html)
within a CMakeLists.txt file. See the examples below for typical usage scenarios.

### Example Using OpenDDS Source Tree

To generate/compile the Messenger examples referenced [above](#cmake-messenger-examples)
(assuming `source setenv.sh` on Unix or `setenv.cmd` on Windows was already run) within
the downloaded and compiled OpenDDS source-tree:

#### Unix

```bash
cd $DDS_ROOT/tests/cmake_integration/Messenger/Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$DDS_ROOT ..
cmake --build .
```

#### Windows

The following assumes Visual Studio 2017 using 64-bit architecture (in
case it's not obvious from the -G switch).

```bat
cd %DDS_ROOT%\tests\cmake_integration\Messenger\Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=%DDS_ROOT:\=/% -G "Visual Studio 15 2017 Win64" ..
cmake --build .
```
*Note:* The `:\=/` portion of the `%DDS_ROOT:\=/%` is required to prevent
backslashes from being passed into cmake (which might cause an allergic reaction).

### Example Using Installed OpenDDS (Unix only)

After OpenDDS has been downloaded, the `--prefix` switch can be passed into
configure to enable the `install` target, which will copy everything required
for a working OpenDDS installation when built (including the OpenDDS CMake
Config module) into the specified directory.

Here is a snippet to download OpenDDS, compile, install, and generate/compile
the Messenger_1 example from outside the source-tree:

```bash
git clone git@github.com:objectcomputing/OpenDDS.git $HOME/opendds
cd $HOME/opendds
./configure --prefix=$HOME/opendds-install --ace-github-latest
make
make install
cp -ar $HOME/opendds/tests/cmake_integration/Messenger $HOME/Messenger
cd $HOME/Messenger/Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$HOME/opendds-install ..
cmake --build .
```
*Note:* The `--ace-github-latest` switch will download the latest ACE/TAO sources
from github, as opposed to downloading the most-recently released version.

*Note:* While this will build the Messenger_1 example, the run_test.pl script will
not work due to missing perl dependencies (well, they are in the source-tree but
that is out of scope).

## Adding IDL Sources with OPENDDS_TARGET_SOURCES

The other benefit provided by the OpenDDS Config Package (aside from importing the
various OpenDDS targets) is an easy way to add IDL sources to CMake targets. This
is achieved by the `OPENDDS_TARGET_SOURCES` macro, which behaves similarly to the
built-in [`target_sources`](https://cmake.org/cmake/help/latest/command/target_sources.html)
command save for the following differences:

  - Items can be either C/C++ sources or IDL sources.
  - The scope-qualifier (PUBLIC, PRIVATE, INTERFACE) is not required.
    When it is omitted, PRIVATE is used by default.
  - Command-line options can be supplied to the TAO/OpenDDS IDL compilers
    using TAO_IDL_OPTIONS and/or OPENDDS_IDL_OPTIONS (if the default
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

Taken from the Messenger_1 example referenced above, here is a snippet showcasing
how IDL files can be added directly to executable targets using the
`OPENDDS_TARGET_SOURCES` macro:

```cmake
add_executable(publisher
    ${src}/publisher.cpp
)
OPENDDS_TARGET_SOURCES(publisher
  PUBLIC
    ${src}/Writer.cpp
    ${src}/Writer.h
    ${src}/Messenger.idl
)
```
