# OpenDDS and iOS

**How to build OpenDDS for iOS and incorporate OpenDDS into iOS apps.**

- [OpenDDS and iOS](#opendds-and-ios)
  - [Variables](#variables)
  - [Requirements](#requirements)
  - [Building on macOS](#building-on-macos)
    - [Host Tools](#host-tools)
    - [OpenDDS's Optional Dependencies](#openddss-optional-dependencies)
      - [OpenSSL](#openssl)
      - [Xerces](#xerces)
  - [Cross-Compiling IDL Libraries](#cross-compiling-idl-libraries)
  - [Using OpenDDS in an iOS App](#using-opendds-in-an-ios-app)
   
## Variables

The following table describes some of the variables of paths that are
referenced in this guide. You don't have to actually set and use these, they
are mostly for shorthand.

| Variable        | Description                            |
| --------------- | -------------------------------------- |
| `$DDS_ROOT`     | OpenDDS being built for Android        |
| `$ACE_ROOT`     | ACE being built for Android            |
| `$IPHONE_TARGET`| Set to 'SIMULATOR' or 'HARDWARE'       |

## Requirements

To build the core OpenDDS native libraries for iOS you will need:

 - A macOS development system with Xcode and Xcode command-line tools.
 - Some knowledge about OpenDDS development and iOS development. 

Building OpenDDS with optional dependencies has additional requirements 
listed in their own sections.

The [\"Using OpenDDS in an iOS App\"](#using-opendds-in-an-ios-app) section assumes the use of Xcode.

## Building on macOS

This version of OpenDDS has been tested on iOS using Xcode 11, arm64
iPhones (iPhone 5s and later), and x86_64 iOS simulators.

The OpenDDS configure scripts support simulator and hardware builds through 
the `IPHONE_TARGET` environment variable.

**NOTE**: OpenDDS Java bindings are not supported on iOS, since iOS does not
have a Java runtime. To use [DDS Security](#openssl) with iOS, read the [OpenDDS's 
Optional Dependencies](#openddss-optional-dependencies) sections before configuring and building OpenDDS.

To configure OpenDDS for an iOS simulator build, run the configure script in
`$DDS_ROOT` from a bash shell:

```Shell
./configure --ace-github-latest --host=macosx --target=ios --std=c++11 --macros=IPHONE_TARGET=SIMULATOR
```

Configuring a hardware build is similar:

```Shell
./configure --ace-github-latest --host=macosx --target=ios --std=c++11 --macros=IPHONE_TARGET=HARDWARE
```

Then run make to build the host tools and the target static libraries:

```Shell
make
```

### Host Tools

To cross-compile OpenDDS, host tools are required to process IDL. The example
above generates two copies of OpenDDS, one in `OpenDDS/build/host` and another
in `OpenDDS/build/target`. If this is the case, then `$HOST_DDS` will be the
absolute path to `build/host` and `$DDS_ROOT` will be the absolute path to
`build/target`.

If building for both iOS simulators and iPhones, it might make sense to build
the OpenDDS host tools separately to cut down on compile time and disk space.

If this is the case, then `$HOST_DDS` will be the location of the static host
tools built for the host platform and `$DDS_ROOT` will just be the location of
the OpenDDS source code.

This should be done with the same version of OpenDDS and ACE/TAO as what you
want to build iOS. Pass `--host-tools-only` to the configure script to
generate static host tools. 

If you want to just the minimum needed for host OpenDDS tools and get rid of
the rest of the source files, you can. These are the binaries that make up the
OpenDDS host tools:

 * `$HOST_DDS/bin/opendds_idl`
 * `$HOST_DDS/ACE_TAO/bin/ace_gperf`
 * `$HOST_DDS/ACE_TAO/bin/tao_idl`

These files can be separated from the rest of the OpenDDS and ACE/TAO source
trees, but the directory structure must be kept. To use these to build OpenDDS
for iOS, pass `--host-tools $HOST_DDS` to the configure script.

### OpenDDS's Optional Dependencies

#### OpenSSL

OpenSSL is required for OpenDDS Security. To configure and build OpenSSL for 
iPhone targets, use the `ios64-cross` configuration flag, and set the following
environment variables:

```Shell
export CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
export CROSS_TOP=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer
export CROSS_SDK=iPhoneOS.sdk
export PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin:$PATH"
```
Run the configure script:
  
```Shell
./Configure ios64-cross no-shared no-dso no-hw no-engine --prefix=/usr/local/ios/openssl
```

Build with make:

```Shell
make
```

Install with make:

```Shell
make install
```

Set `OPENSSL_ROOT` to the install location:

```Shell
export OPENSSL_ROOT=/usr/local/ios/openssl
```

Note that the directory given by `--prefix=` will be created by `make install`
and will have `include` and `lib` subdirectories that will be used by the 
OpenSSL build. 

OpenSSL builds for iOS simulators are not as well-supported, and require 
a minor modification to the generated makefile. To build OpenSSL for iOS
simulators, set the following environment variables:

```Shell
export CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
export CROSS_TOP=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer
export CROSS_SDK=iPhoneSimulator.sdk
export PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin:$PATH"
```
Run the configure script in openssl-1.1.1d/:
```Shell
./Configure darwin64-x86_64-cc no-shared no-dso no-hw no-engine --prefix=/usr/local/ios/openssl
```

Modify the generated Makefile setting CNF_CFLAGS, and CNF_CXXFLAGS to:

```Shell
CNF_CFLAGS=-arch x86_64 -miphoneos-version-min=12.0 -isysroot $(CROSS_TOP)/SDKs/$(CROSS_SDK)
```

Build with make:

```Shell
make
```

Install with make:

```Shell
make install
```

Set `OPENSSL_ROOT` to the install location:

```Shell
export OPENSSL_ROOT=/usr/local/ios/openssl
```

Note that the directory given by `--prefix=` will be created by `make install`
and will have `include` and `lib` subdirectories that will be used by the 
OpenSSL build. 

OpenDDS security builds need the location of OpenSSL installed headers and
static libraries. This location can be passed to the OpenDDS configure script 
with the `--openssl=\${OPENSSL_ROOT}` flag.

#### Xerces

Xerces C++ is also required for OpenDDS Security. Xerces builds for iOS use 
cmake and require passing the cross-compile flags needed by the macOS C and 
C++ compilers as well as setting the iOS SDK root and other build flags.
All setting can be passed on the command line. 

A representative simulator build can be configured as:

```Shell
CC="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang -miphoneos-version-min=12.0 -arch x86_64" CXX="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -miphoneos-version-min=12.0 -arch x86_64" cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -Dnetwork:BOOL=OFF -Dtranscoder=iconv -DCMAKE_INSTALL_PREFIX=/usr/local/ios/xerces3 -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
```

After configuring the build, run cmake:

```Shell
cmake --build src
```

Install with make:

```Shell
make install
```

Set `$XERCES_ROOT` to the install location:

```Shell
export XERCES_ROOT=/usr/local/ios/xerces3
```

Note that the directory given by `--prefix=` will be created by `make install`
and will have `include` and `lib` subdirectories that will be used by the Xerces
build. 

Configuring Xerces for a iPhone hardware build is similar to the simulator 
build. The `--arch` flag changes to `arm64` and the `CMAKE_OS_SYSROOT` changes to 
the location of the iPhone SDK.

```Shell
CC="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang -miphoneos-version-min=12.0 -arch arm64" CXX="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -miphoneos-version-min=12.0 -arch arm64" cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -Dnetwork:BOOL=OFF -Dtranscoder=iconv -DCMAKE_INSTALL_PREFIX=/usr/local/ios/xerces3 -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
```

OpenDDS security builds need the location of Xerces installed headers and
static libraries. This location can be passed to the OpenDDS configure script with
the `--xerces3=${XERCES_ROOT}` flag.

## Cross-Compiling IDL Libraries

Like all OpenDDS applications, you will need to use type support libraries
generated from IDL files to use most of OpenDDS's functionality.

Assuming the library is already setup and works for a desktop platform, then
you should be able to run the DDS environment setup script from the
`OpenDDS/builds/target` directory to set the appropriate iOS build flags:

```Shell
(source $DDS_ROOT/setenv.sh; mwc.pl -type gnuace . && make)
```

The resulting native IDL library file must be included with the rest of the
native library files.

## Using OpenDDS in an iOS App

Copy the static libraries in `$ACE_ROOT/lib`, `$DDS_ROOT/lib`, and the 
cross-compiled IDL libraries (and optionally `$OPENSSL_ROOT/lib` and 
`$XERCES_ROOT/lib`) to the Xcode project or framework directory.

In the Xcode project, add `$ACE_ROOT` and `$DDS_ROOT` the header search paths.
