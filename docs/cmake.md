# Using OpenDDS in a CMake Project

OpenDDS can be used with CMake-Based projects by using the OpenDDS CMake Config
package (located [here](../cmake)). This package bridges the gap between the
MPC build system used by OpenDDS and CMake-Based projects by exposing OpenDDS CMake
targets (see comments [here](../cmake/OpenDDSConfig.cmake)), and by providing the
[`OPENDDS_TARGET_SOURCES`](#adding-idl-sources-with-opendds_target_sources) macro
which simplifies IDL compilation.

**Table of Contents:**

* [Requirements](#requirements)
* [CMake Messenger Examples](#cmake-messenger-examples)
* [Configure-Generated Variables](#configure-generated-variables)
* [Using the OpenDDS CMake Package](#using-the-opendds-cmake-package)
  * [Cache Variables/Options Understood by OpenDDS](#cache-variablesoptions-understood-by-opendds)
  * [Example Using OpenDDS Source Tree](#example-using-opendds-source-tree)
    * [Unix](#unix)
    * [Windows](#windows)
  * [Example Using Installed OpenDDS (Unix only)](#example-using-installed-opendds-unix-only)
* [Adding IDL Sources with `OPENDDS_TARGET_SOURCES`](#adding-idl-sources-with-opendds_target_sources)
  * [Usage Summary](#usage-summary)
  * [Example](#example)
* [Advanced Usage](#advanced-usage)
  * [Manually Creating config.cmake](#manually-creating-configcmake)
    * [Optional OpenDDS Features](#optional-opendds-features)
    * [Build-Related Options](#build-related-options)
  * [`OPENDDS_DEFAULT_NESTED`](#opendds_default_nested)

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
`config.cmake`  will be placed in
`/path/to/install-prefix-dir/lib/cmake/OpenDDS`.  This location allows CMake to
locate the file using `CMAKE_PREFIX_PATH`.

Not all of the variables in this file are used.  They are procedurally
generated from variables in the configure script. However, to get a better
sense of how they are being used take a look at the
[options.cmake](../cmake/options.cmake) file. This is where various
CMake C/C++ compile-time defines and IDL-Compiler flags are set/unset
depending upon which variables are enabled/disabled.

If you are using OpenDDS libraries that were built without the help of the
`configure` script, the `config.cmake` file needs to be created manually.
See [Manually Creating config.cmake](#manually-creating-configcmake)
in the advanced section below.

## Using the OpenDDS CMake Package

The OpenDDS CMake package is based upon the standard CMake
[packaging guidelines](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html),
so the typical generate/compile steps only require setting the `CMAKE_PREFIX_PATH` and
calling [`find_package(OpenDDS)`](https://cmake.org/cmake/help/latest/command/find_package.html)
within a CMakeLists.txt file. In addition, the following cache variables/options can be used to
control the behavior of the OpenDDS CMake package.

### Cache Variables/Options Understood by OpenDDS

| Cache Variable              | Description                                                         | Default |
| --------------------------- | ------------------------------------------------------------------- | ------- |
| `OPENDDS_CMAKE_VERBOSE`     | Print detailed status information at CMake-Generation time          | `OFF`   |
| `OPENDDS_DEFAULT_NESTED`    | [Topic types must be declared explicitly.](#opendds_default_nested) | `ON`    |

### Example Using OpenDDS Source Tree

When using the CMake Package, the OpenDDS build environment variables (from
setenv.sh or setenv.cmd) do not need to be set.  CMake's varaible
`CMAKE_PREFIX_PATH` needs to include the top directory of OpenDDS's source tree,
that is the directory known as `DDS_ROOT` when OpenDDS was compiled.

To generate/compile the [Messenger with direct IDL inclusion] within the OpenDDS source tree:

#### Unix

```bash
cd tests/cmake_integration/Messenger/Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=<location of OpenDDS source tree> ..
cmake --build .
```

#### Windows

The following assumes Visual Studio 2017 using 64-bit architecture (adjust the
CMake `-G` parameter if using something different).

```bat
cd tests\cmake_integration\Messenger\Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=<location of OpenDDS source tree> -G "Visual Studio 15 2017 Win64" ..
cmake --build .
```

### Example Using Installed OpenDDS (Unix only)

The `--prefix` switch can be passed to configure to enable the `install` target,
which will install OpenDDS (including the OpenDDS CMake Config module) into the
specified directory.  See the [INSTALL](../INSTALL) document for details.

*Note:* Be sure to pass an absolute path to `--prefix`.

```bash
DDS_WORKSPACE=$(pwd)
cd OpenDDS-src
./configure --prefix=$DDS_WORKSPACE/opendds-install
make
make install
cd tests/Messenger/Messenger_1
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=$DDS_WORKSPACE/opendds-install ..
cmake --build .
```
*Note:* While this will build the Messenger\_1 example, the `run_test.pl` script will
not work due to missing Perl dependencies.

## Adding IDL Sources with `OPENDDS_TARGET_SOURCES`

Aside from importing the various OpenDDS targets, the OpenDDS Config Package
provides an easy way to add IDL sources to CMake targets. This is achieved by
the `OPENDDS_TARGET_SOURCES` macro, which behaves similarly to the
built-in [`target_sources`](https://cmake.org/cmake/help/latest/command/target_sources.html) command except for the following:

  - Items can be either C/C++ sources or IDL sources.
  - The scope-qualifier (`PUBLIC`, `PRIVATE`, `INTERFACE`) is not required.
    When it is omitted, `PRIVATE` is used by default.
  - Command-line options can be supplied to the TAO/OpenDDS IDL compilers
    using `TAO_IDL_OPTIONS` and/or `OPENDDS_IDL_OPTIONS` (if the default
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
  [OPENDDS_IDL_OPTIONS options...]
)
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

Another snippet, based upon [Messenger with auxiliary IDL library], showcases how an
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

## Advanced Usage

### Manually Creating config.cmake

In order to work with config.cmake it is important to understand the purpose
of its contained variables. Below is a list of the important variables with
some information on their purpose.

#### Optional OpenDDS Features
When OpenDDS is built using the MPC build-system
and optional features are specified, various C/C++; tao_idl; and opendds_idl
compiler macros are set on dependent libraries or executables. These defines
are set by MPC using the [dcps_optional_features.mpb](../MPC/config/dcps_optional_features.mpb)
file.

CMake-Based targets must also inherit the defined macros, so the functionality
in `dcps_optional_features.mpb` is simulated using `options.cmake`
and its associated `config.cmake`. As such, if features are enabled/disabled when
compiling OpenDDS then it is paramount that the MPC features are also matched
within `config.cmake` prior to compilation.

This table outlines the feature-related variables and their _expected_ default
values (generated when the configure script is run without arguments).
If any feature is explicitly enabled/disabled in the OpenDDS build then the corresponding
CMake variable below should be set to either _ON_ or _OFF_ depending on the desired state.

| CMake Variable                   | Notes (from configure script output)    | Default |
|---                               | ---                                     | ---     |
|`OPENDDS_BUILT_IN_TOPICS`         | Built-in Topics                         | `ON`      |
|`OPENDDS_CONTENT_FILTERED_TOPIC`  | ContentFilteredTopic (CS Profile)       | `ON`      |
|`OPENDDS_CONTENT_SUBSCRIPTION`    | Content-Subscription Profile. When this is set to `OFF` then `CONTENT_FILTERED_TOPIC`, `MULTI_TOPIC`, and `QUERY_CONDITION` will also be set to `OFF`. See the [OpenDDS Dev Guide] ch. 5 for info. | `ON`      |
|`OPENDDS_MULTI_TOPIC`             | MultiTopic (CS Profile)                 | `ON`      |
|`OPENDDS_OBJECT_MODEL_PROFILE`    | Object Model Profile                    | `ON`      |
|`OPENDDS_OWNERSHIP_KIND_EXCLUSIVE`| Exclusive Ownership (Ownership Profile) | `ON`      |
|`OPENDDS_OWNERSHIP_PROFILE`       | Ownership Profile. When this is set to `OFF` then `OWNERSHIP_KIND_EXCLUSIVE` will also be set to `OFF`. | `ON` |
|`OPENDDS_PERSISTENCE_PROFILE`     | Persistence Profile                     | `ON`      |
|`OPENDDS_QUERY_CONDITION`         | QueryCondition (CS Profile)             | `ON`      |
|`OPENDDS_SECURITY`                | DDS Security plugin                     | `OFF`     |

#### Build-Related Options

The following values impact the build in one way or another.

| CMake Variable            | Notes                                                              | Default             |
|---                        | ---                                                                | ---                 |
|`OPENDDS_ACE`              | Location of ACE root dir.                                          | N/A                 |
|`OPENDDS_TAO`              | Location of TAO root dir.                                          | N/A                 |
|`OPENDDS_STD`              | Forces C++ standard (Unix only). This option is used as a way for the configure script to inform CMake builds of the C/C++ standard used to build OpenDDS. To prevent weirdness, the C/C++ standards should match. The typical GCC `-std` values are supported. | Existing `CMAKE_CXX_STANDARD` value. |
|`OPENDDS_NO_DEBUG`         | Sets NDEBUG flags on ACE for non-debug builds (Unix only)          | `OFF`                 |
|`OPENDDS_INLINE`           | ACE's inline build flag                                            | See below           |
|`OPENDDS_STATIC`           | Use static libraries                                               | `OFF`                 |
|`OPENDDS_XERCES3`          | Adds dependencies to targets; required when `OPENDDS_SECURITY` is `ON` | `OFF`                 |
|`OPENDDS_FEATURES`         | Semicolon-Separated list of additional features which impact the build. Currently supported are `versioned_namespace=1` (see [this](https://github.com/DOCGroup/ACE_TAO/blob/master/ACE/docs/Symbol_Versioning.html) document) and `uses_wchar=1` for wide-character support. | N/A |

`OPENDDS_INLINE` should be explicitly set to `ON` or `OFF` (based on the ACE `platform_macros.GNU` variable `inline`) in `config.cmake` unless you will only be using a CMake Microsoft Visual Studio Generator.

[Messenger with direct IDL inclusion]: ../tests/cmake_integration/Messenger/Messenger_1/CMakeLists.txt
[Messenger with auxiliary IDL library]: ../tests/cmake_integration/Messenger/Messenger_2/CMakeLists.txt
[OpenDDS Dev Guide]: http://opendds.org/documents/

### `OPENDDS_DEFAULT_NESTED`

When parsing IDL, OpenDDS will determine if it should be prepared for a
`struct` or `union` to be used in a topic if its "nested" value is false. See
the OpenDDS Developer's Guide for more information.

`OPENDDS_DEFAULT_NESTED` sets the global default for what the "nested" property
is. If this value is `ON`, then topic types for a given project must be
declared using annotations. If it's `OFF`, then every valid type is assumed to
be needed for use as a topic type by default, which might add code to the IDL
type support libraries that will never be used.

This default can also be controlled on a finer level when calling
`OPENDDS_TARGET_SOURCES` by passing `--default-nested` or `--no-default-nested`
to `OPENDDS_IDL_OPTIONS`. For example:

```cmake
add_library(messenger)
OPENDDS_TARGET_SOURCES(messenger
  Messenger.idl
  OPENDDS_IDL_OPTIONS --no-default-nested
)
```
