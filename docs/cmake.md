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
  * [Developer's Guide Messenger Example](#developers-guide-messenger-example)
  * [Advanced Examples](#advanced-examples)
* [Configure-Generated Variables](#configure-generated-variables)
* [Using the OpenDDS CMake Package](#using-the-opendds-cmake-package)
  * [`find_package`](#find_package)
  * [Cache Variables/Options Understood by OpenDDS](#cache-variablesoptions-understood-by-opendds)
  * [Libraries](#libraries)
  * [Building the Developer's Guide Messenger Example](#building-the-developers-guide-messenger-example)
    * [Unix](#unix)
    * [Windows](#windows)
    * [Example Using Installed OpenDDS (Unix only)](#example-using-installed-opendds-unix-only)
* [Adding IDL Sources with `OPENDDS_TARGET_SOURCES`](#adding-idl-sources-with-opendds_target_sources)
  * [Example](#example)
* [Advanced Usage](#advanced-usage)
  * [Manually Creating config.cmake](#manually-creating-configcmake)
    * [Optional OpenDDS Features](#optional-opendds-features)
    * [Build-Related Options](#build-related-options)
  * [`OPENDDS_DEFAULT_NESTED`](#opendds_default_nested)
  * [`OPENDDS_FILENAME_ONLY_INCLUDES`](#opendds_filename_only_includes)
  * [`OPENDDS_TARGET_SOURCES` Target Properties](#opendds_target_sources-target-properties)
    * [`OPENDDS_LANGUAGE_MAPPINGS`](#opendds_language_mappings)
    * [`OPENDDS_GENERATED_DIRECTORY`](#opendds_generated_directory)
    * [`OPENDDS_*_INTERFACE_FILES`](#opendds__interface_files)
  * [`install`-related Tasks](#install-related-tasks)
    * [`install(IMPORTED_RUNTIME_ARTIFACTS)`](#installimported_runtime_artifacts)
    * [Installing Generated Interface Files](#installing-generated-interface-files)

## Requirements

In order to use the Config module, CMake version 3.3.2 or greater is required.
However, to run the Messenger examples, version 3.8.2 or greater is required
(due to some dependencies which simplify the custom file-copy targets).

## CMake Messenger Examples

### Developer's Guide Messenger Example

For a simple quick-start example of an `CMakeLists.txt` using OpenDDS see the
[Developer's Guide Messenger example].

### Advanced Examples

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
`config.cmake` file (placed in [`$DDS_ROOT/cmake`](../cmake)),
which has various configuration options. These options provide the OpenDDS
CMake package with the required context it needs to integrate with the
OpenDDS code generators and libraries.

*Note:* With an installed version of OpenDDS (generated using `make install`),
`config.cmake` will be placed in `$INSTALL_PREFIX/lib/cmake/OpenDDS`.

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

### `find_package`

To use the OpenDDS CMake Package, first it has to be loaded using
[`find_package`](https://cmake.org/cmake/help/latest/command/find_package.html).
For example to mandatorily load OpenDDS:

```cmake
find_package(OpenDDS REQUIRED)
```

For this to work, CMake has to be able to find the package. If the OpenDDS
build environment variables (from `source setenv.sh` or `call setenv.cmd`) are
set then CMake should be able to find it using the `PATH` environment variable.
If those environment variables aren't set, OpenDDS was installed to a path
CMake doesn't search automatically, or CMake can't otherwise find the OpenDDS
package, then CMake has to be told about it explicitly somehow. This can be
done a number of ways, which includes adding the OpenDDS source tree or install
prefix path to `CMAKE_PREFIX_PATH` or setting `OPENDDS_ROOT` to that path (if
using CMake 3.12 or later).

Consult the `find_package` documentation for your CMake version for all the
details on how CMake could find OpenDDS.
[Here](https://cmake.org/cmake/help/latest/command/find_package.html#config-mode-search-procedure)
is the documentation for the latest version of CMake

### Cache Variables/Options Understood by OpenDDS

The following cache variables/options can be used to control the behavior of
the OpenDDS CMake package.

| Cache Variable                   | Description                                                                  | Default   |
| -------------------------------- | ---------------------------------------------------------------------------- | --------- |
| `OPENDDS_CMAKE_VERBOSE`          | Print detailed status information at CMake-Generation time                   | `OFF`     |
| `OPENDDS_DEFAULT_NESTED`         | [Topic types must be declared explicitly.](#opendds_default_nested)          | `ON`      |
| `OPENDDS_FILENAME_ONLY_INCLUDES` | [No directory info in generated #includes.](#opendds_filename_only_includes) | `OFF`     |
| `OPENDDS_DEFAULT_SCOPE`          | Default scope of unscoped files in `OPENDDS_TARGET_SOURCES`                  | `PRIVATE` |
| `OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER` | Default for `ALWAYS_GENERATE_LIB_EXPORT_HEADER` in `OPENDDS_TARGET_SOURCES` | `OFF`|

### Libraries

The following libraries are some of what can be defined by the package:

 - `OpenDDS::Dcps`
   - Core OpenDDS Library
 - `OpenDDS::Rtps`
   - RTPS Discovery
 - `OpenDDS::InfoRepoDiscovery`
   - InfoRepo Discovery
 - `OpenDDS::Rtps_Udp`
   - RTPS Transport
 - `OpenDDS::Multicast`
   - Multicast Transport
 - `OpenDDS::Shmem`
   - Shared Memory Transport
 - `OpenDDS::Tcp`
   - TCP Transport
 - `OpenDDS::Udp`
   - UDP Transport
 - `OpenDDS::Security`
   - DDS Security Implementation, if Available
 - `OpenDDS::OpenDDS`
   - A Combination of all the Previous Libraries

See [OpenDDSConfig.cmake](../cmake/OpenDDSConfig.cmake) for all the libraries
and more details on them.

### Building the Developer's Guide Messenger Example

The following instructions show how to configure and build the [Developer's
Guide Messenger example]. Make sure the environment is setup by using `call
setenv.cmd` on Windows or `source setenv.sh` elsewhere.

#### Unix

```bash
cd DevGuideExamples/DCPS/Messenger
mkdir build
cd build
cmake ..
cmake --build .
perl run_test.pl
```

#### Windows

The following assumes Visual Studio 2017 using 64-bit architecture (adjust the
CMake `-G` parameter if using something different).

```bat
cd DevGuideExamples\DCPS\Messenger
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
cmake --build .
perl run_test.pl
```

#### Example Using Installed OpenDDS (Unix only)

The `--prefix` switch can be passed to configure to enable the `install` target,
which will install OpenDDS (including the OpenDDS CMake Config module) into the
specified directory. See the [INSTALL.md](../INSTALL.md) document for details.

*Note:* Be sure to pass an absolute path to `--prefix`.

```bash
OPENDDS_PREFIX="$PWD/opendds-install"
DDS_ROOT="$PWD/OpenDDS"
ACE_ROOT="$DDS_ROOT/ACE_wrappers"
cd OpenDDS
./configure --prefix="$OPENDDS_PREFIX"
make -j $(getconf _NPROCESSORS_ONLN)
make install
cd DevGuideExamples/DCPS/Messenger
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="$OPENDDS_PREFIX" ..
cmake --build .
PERL5LIB="$DDS_ROOT/bin:$ACE_ROOT/bin" LD_LIBRARY_PATH="$OPENDDS_PREFIX/lib:$LD_LIBRARY_PATH" perl run_test.pl
```

## Adding IDL Sources with `OPENDDS_TARGET_SOURCES`

Aside from importing the various OpenDDS targets, the OpenDDS Config Package
provides an easy way to add IDL sources to CMake targets. This is achieved by
the `OPENDDS_TARGET_SOURCES` macro, which was inspired by the built-in
`target_sources`:

```
OPENDDS_TARGET_SOURCES(<target>
  [<idl file>...]
  [INTERFACE|PUBLIC|PRIVATE <idl file>...]...
  [TAO_IDL_OPTIONS <option>...]
  [OPENDDS_IDL_OPTIONS <option>...]
  [SUPPRESS_ANYS ON|OFF]
  [ALWAYS_GENERATE_LIB_EXPORT_HEADER ON|OFF]
)
```

`<idl file>...` are IDL files that can be absolute or relative to
`CMAKE_CURRENT_SOURCE_DIR`. Each one will generate code using `tao_idl` and
`opendds_idl` that is added to the target. C/C++ items can also be accepted and
added to the target, but passing non-IDL sources was deprecated in OpenDDS
3.15. The scope-qualifier (`PUBLIC`, `PRIVATE`, `INTERFACE`) sets the scope of
the generated files. When it is omitted, `OPENDDS_DEFAULT_SCOPE` is used, which
is `PRIVATE` by default.

Command-line options can be supplied to the TAO/OpenDDS IDL compilers using
`TAO_IDL_OPTIONS` and/or `OPENDDS_IDL_OPTIONS` (if the default behavior is not
suitable). Add `OPENDDS_IDL_OPTIONS -Lc++11` to use the C++11 IDL Mapping.

An option is available to force creation of typecodes by using `SUPPRESS_ANYS
OFF`. This value will overrule the one received from config.cmake
`OPENDDS_SUPPRESS_ANYS`.

If the passed-in target is a shared library, a custom command will also be
added to generate the required IDL export header file (`TARGET_export.h`),
which is necessary to properly export symbols for the IDL-Generated sources.
The `ALWAYS_GENERATE_LIB_EXPORT_HEADER` argument forces this to be true for all
libraries types. This is only really useful if the target is a library that
uses the export header itself and also needs to be built as a static library as
well. This way the export library can be included unconditionally. The default
for this is set by the `OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER`, which
defaults to `OFF`.

See ["`OPENDDS_TARGET_SOURCES` Target Properties"](#opendds_target_sources-target-properties)
for the target properties that are set on the target.

### Example

Based on the [Messenger with direct IDL inclusion], here is a snippet showing
how IDL files can be added directly to executable targets using the `OPENDDS_TARGET_SOURCES`
macro:

```cmake
add_executable(publisher
  "publisher.cpp"
  "Writer.cpp"
)
OPENDDS_TARGET_SOURCES(publisher "Messenger.idl")
target_link_libraries(publisher OpenDDS::OpenDDS)
```

This is fine for small IDL files and builds with a few targets that need the
code generated from IDL, but it can be improved upon.

Another snippet, based upon [Messenger with auxiliary IDL library], showcases how an
auxiliary IDL library can be created for inclusion by other executables:

```cmake
add_library(messenger)
OPENDDS_TARGET_SOURCES(messenger PUBLIC "Messenger.idl")
target_link_libraries(messenger OpenDDS::Dcps)

add_executable(publisher
  "publisher.cpp"
  "Writer.cpp"
)
target_link_libraries(publisher OpenDDS::OpenDDS)
```

Here the generated code from the IDL is only compiled once. This is especially
advantageous if you have a significant amount of IDL and/or have many targets
that require the IDL. Additionally if the library is a shared library,
executables can share that file and this reduces the size of the executable
files.

*Note:* CMake version 3.10 and below will issue a harmless warning if
`add_library` is called without any sources.

## Advanced Usage

### Manually Creating config.cmake

In order to work with config.cmake it is important to understand the purpose
of its contained variables. Below is a list of the important variables with
some information on their purpose.

#### Optional OpenDDS Features

When OpenDDS is built using the MPC build-system
and optional features are specified, various C/C++; `tao_idl`; and `opendds_idl`
compiler macros are set on dependent libraries or executables. These defines
are set by MPC using the [`dcps_optional_features.mpb`](../MPC/config/dcps_optional_features.mpb)
file.

CMake-Based targets must also inherit the defined macros, so the functionality
in `dcps_optional_features.mpb` is simulated using `options.cmake`
and its associated `config.cmake`. As such, if features are enabled/disabled when
compiling OpenDDS then it is paramount that the MPC features are also matched
within `config.cmake` prior to compilation.

This table outlines the feature-related variables and their _expected_ default
values (generated when the configure script is run without arguments).
If any feature is explicitly enabled/disabled in the OpenDDS build then the corresponding
CMake variable below should be set to either `ON` or `OFF` depending on the desired state.

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
|`OPENDDS_SAFETY_PROFILE`          | Safety Profile                          | `OFF`     |

#### Build-Related Options

The following values impact the build in one way or another.

| CMake Variable            | Notes                                                              | Default             |
|---                        | ---                                                                | ---                 |
|`OPENDDS_ACE`              | Location of ACE root dir.                                          | N/A                 |
|`OPENDDS_TAO`              | Location of TAO root dir.                                          | N/A                 |
|`OPENDDS_INLINE`           | ACE's inline build flag                                            | See below           |
|`OPENDDS_STATIC`           | Use static libraries                                               | `OFF`                 |
|`OPENDDS_XERCES3`          | Adds dependencies to targets; required when `OPENDDS_SECURITY` is `ON` | `OFF`                 |
|`OPENDDS_CXX11`            | ACE/TAO and OpenDDS were built with C++11 or later.                | N/A                 |
|`OPENDDS_WCHAR`            | ACE/TAO and OpenDDS were built to prefer wide characters.          | `OFF`               |
|`OPENDDS_VERSIONED_NAMEPSACE` | ACE/TAO and OpenDDS have versioned namespaces.                  | `OFF`               |
|`OPENDDS_FEATURES`         | List of additional features which impact the build.                | N/A                 |

`OPENDDS_INLINE` should be explicitly set to `ON` or `OFF` (based on the ACE `platform_macros.GNU` variable `inline`) in `config.cmake` unless you will only be using a CMake Microsoft Visual Studio Generator.

[Developer's Guide Messenger example]: ../DevGuideExamples/DCPS/Messenger/CMakeLists.txt
[Messenger with direct IDL inclusion]: ../tests/cmake/Messenger/Messenger_1/CMakeLists.txt
[Messenger with auxiliary IDL library]: ../tests/cmake/Messenger/Messenger_2/CMakeLists.txt
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
  PUBLIC
    Messenger.idl
    OPENDDS_IDL_OPTIONS --no-default-nested
)
```

### `OPENDDS_FILENAME_ONLY_INCLUDES`

This setting tells OpenDDS's IDL compiler to strip path information from `#include`
lines in generated files. Turning the option on can make it easier to specify build
rules for IDL files that include other IDL files.

### `OPENDDS_TARGET_SOURCES` Target Properties

After `OPENDDS_TARGET_SOURCES` is run on a target, it will have these
properties set.

#### `OPENDDS_LANGUAGE_MAPPINGS`

This signifies the IDL language mappings available in the generated IDL
bindings built into the target based on what is passed to
`OPENDDS_IDL_OPTIONS`. It will be a list that can contain one or more of the
following:

 - `"C++03"`
   - Default IDL-to-C++ mapping generated by default.
 - `"C++11"`
   - IDL-to-C++11 mapping available when passing `-Lc++11`.
 - `"FACE"`
   - Will appear if `-Lface` is passed.
 - `"Java"`
   - Currently unsupported in the CMake module, but this will appear in the
     list if `-Wb,java` is passed.

If the CMake version is at least 3.12, then this property will exported with
the target.

#### `OPENDDS_GENERATED_DIRECTORY`

The is the directory where generated files have been placed. This is an
absolute path and is not exported with the target. This can be used along with
the `OPENDDS_*_INTERFACE_FILES` properties to help
[install the generated interface files](installing-generated-interface-files).

#### `OPENDDS_*_INTERFACE_FILES`

These properties are lists of the `PUBLIC` and `INTERFACE` scoped IDL files
that were passed in the files generated based on those.

Some things to note about these:
- All paths are absolute.
- All the generated files will be somewhere within the path from the
  `OPENDDS_GENERATED_DIRECTORY` target property of the target.
- All the properties have the `INTERFACE` in their name, but this includes
  `PUBLIC` scoped files as `PUBLIC` implies `INTERFACE` in CMake. `PRIVATE`
  scoped files are excluded from these lists as they shouldn't have a use
  outside the target.
- These properties are not exported with the target because those paths may not
  be valid any more if the build directory has been removed or the export is
  being used on another machine.
- As passing non-IDL files to `OPENDDS_TARGET_SOURCES` is deprecated, those
  files don't show up in these lists even if they are given `PUBLIC` or
  `INTERFACE` scope.

The properties are:

- `OPENDDS_PASSED_IDL_INTERFACE_FILES`
  - The `PUBLIC` and `INTERFACE` scoped IDL files passed to
    `OPENDDS_TARGET_SOURCES`.
- `OPENDDS_GENERATED_IDL_INTERFACE_FILES`
  - The IDL generated from the IDL files in
    `OPENDDS_PASSED_IDL_INTERFACE_FILES`.
- `OPENDDS_ALL_IDL_INTERFACE_FILES`
  - Combination of `OPENDDS_PASSED_IDL_INTERFACE_FILES` and
    `OPENDDS_GENERATED_IDL_INTERFACE_FILES`.
- `OPENDDS_GENERATED_HEADER_FILES`
  - The `.h` and `.inl` files generated by `OPENDDS_ALL_IDL_INTERFACE_FILES`.
- `OPENDDS_ALL_GENERATED_INTERFACE_FILES`
  - Combination of `OPENDDS_GENERATED_IDL_INTERFACE_FILES` and
    `OPENDDS_GENERATED_HEADER_FILES`.
- `OPENDDS_ALL_INTERFACE_FILES`
  - All the `INTERFACE` and `PUBLIC` scoped files that were passed in or
    generated.

The main indented purpose of these properties is to help
[install the generated interface files](installing-generated-interface-files).

### `install`-related Tasks

#### `install(IMPORTED_RUNTIME_ARTIFACTS)`

If using CMake 3.21 or later, it's possible to install shared libraries from
OpenDDS, ACE, and TAO in CMake along side the application using
[`install(IMPORTED_RUNTIME_ARTIFACTS)`](https://cmake.org/cmake/help/latest/command/install.html#install-imported-runtime-artifacts).
This will just install shared libraries, not any development dependencies like
`opendds_idl` or static libraries.

If OpenDDS and ACE/TAO is built with `clang`, the shared libraries might be
missing an `SONAME` entry. Is is
[an issue with ACE/TAO](https://github.com/DOCGroup/ACE_TAO/issues/1790).
If trying to use `install(IMPORTED_RUNTIME_ARTIFACTS)` in this case, it causes
the dynamic linker to ignore the libraries and report that they could not be
found. One workaround is to add `SOFLAGS+=-Wl,-h,$(SONAME)` to
`$ACE_ROOT/include/makeinclude/platform_macros.GNU` before building. This can
be done manually after running the configure script or by passing
`--macros=SOFLAGS+=-Wl,-h,\$\(SONAME\)` to the configure script.

A function called `opendds_get_library_dependencies` is provided to help find
out what libraries need to be installed:

```
opendds_get_library_dependencies(<output_list_var_name> <lib>...)
```

`<lib>...` are the [libraries](#libraries) as individual arguments or lists.
The result will contain all targets passed and their OpenDDS, ACE, and TAO
dependencies. The result is put in a list named by `output_list_var_name`.

See the [install Test] for an example of using this.

#### Installing Generated Interface Files

It is possible to install files from
[`OPENDDS_*_INTERFACE_FILES` target properties](#opendds__interface_files)
for downstream projects to use. See the [install Test] for an example of
installing these files. It uses
[`install(FILES)`](https://cmake.org/cmake/help/latest/command/install.html#files),
but there isn't any restriction on what installation method can be used. For
example, the
[`PUBLIC_HEADER`](https://cmake.org/cmake/help/latest/prop_tgt/PUBLIC_HEADER.html#prop_tgt:PUBLIC_HEADER)
target property could be set on target to the desired files from the interface
lists. Then they installed using
[`install(TARGETS ... PUBLIC_HEADER ...)`](https://cmake.org/cmake/help/latest/command/install.html#installing-targets).

[install Test]: ../tests/cmake/install/library/CMakeLists.txt
