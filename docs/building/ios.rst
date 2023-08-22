###
iOS
###

How to build OpenDDS for iOS and incorporate OpenDDS into iOS apps.

*********
Variables
*********

The following table describes some of the variables of paths that are referenced in this guide.

.. note:: You don't have to actually set and use all of these, they are mostly for shorthand.

.. list-table::
  :header-rows: 1

  * - **Variable**
    - **Description**

  * - ``$DDS_ROOT``
    - OpenDDS being built for iOS

  * - ``$ACE_ROOT``
    - ACE being built for iOS

  * - ``$HOST_DDS``
    - :ref:`OpenDDS host to help build OpenDDS for iOS <ios-host-tools>`

  * - ``$IPHONE_TARGET``
    - Set to ``SIMULATOR`` or ``HARDWARE``

************
Requirements
************

To build the core OpenDDS native libraries for iOS you will need:

- A macOS development system with Xcode and Xcode command-line tools.
- Some knowledge about OpenDDS development and iOS development.

Building OpenDDS with :ref:`optional dependencies has additional requirements <ios-optional-deps>`.

*****************
Building on macOS
*****************

OpenDDS has been tested on iOS using Xcode 11, arm64 iPhones (iPhone 5s and later), and x86_64 iOS simulators.

The OpenDDS configure scripts support simulator and hardware builds through the ``IPHONE_TARGET`` environment variable.

.. note:: If you need to configure OpenDDS with any optional dependencies then read the :ref:`relevant sections <ios-optional-deps>` before configuring and building OpenDDS.

To configure OpenDDS for an iOS simulator build, run the configure script in ``$DDS_ROOT`` from a bash shell:

.. code-block:: shell

  ./configure --host=macosx --target=ios --std=c++11 --macros=IPHONE_TARGET=SIMULATOR

Configuring a hardware build is similar:

.. code-block:: shell

  ./configure --host=macosx --target=ios --std=c++11 --macros=IPHONE_TARGET=HARDWARE

Then run make to build the host tools and the target static libraries:

.. code-block:: shell

  make

.. _ios-host-tools:

Host Tools
~~~~~~~~~~

To cross-compile OpenDDS, host tools are required to process IDL.
These are programs that include :term:`tao_idl` and :term:`opendds_idl` that have to be built to run on the host system, not iOS.
The example above generates two copies of OpenDDS, one in ``OpenDDS/build/host`` and another in ``OpenDDS/build/target``.
If this is the case, then ``$HOST_DDS`` will be the absolute path to ``build/host`` and ``$DDS_ROOT`` will be the absolute path to ``build/target``.

If building for both iOS simulators and iPhones, it might make sense to build the OpenDDS host tools separately to cut down on compile time and disk space.

If this is the case, then ``$HOST_DDS`` will be the location of the static host tools built for the host platform and ``$DDS_ROOT`` will just be the location of the OpenDDS source code.

This should be done with the same version of OpenDDS and ACE/TAO as what you want to build for iOS.
Pass ``--host-tools-only`` to the configure script to generate static host tools.

If you want to just the minimum needed for host OpenDDS tools and get rid of the rest of the source files, you can.
These are the binaries that make up the OpenDDS host tools:

- ``$HOST_DDS/bin/opendds_idl``
- ``$HOST_DDS/ACE_TAO/bin/ace_gperf``
- ``$HOST_DDS/ACE_TAO/bin/tao_idl``

These files can be separated from the rest of the OpenDDS and ACE/TAO source trees, but the directory structure must be kept.
To use these to build OpenDDS for iOS, pass ``--host-tools $HOST_DDS`` to the configure script.

.. _ios-optional-deps:

OpenDDS’s Optional Dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note:: The OpenDDS Java bindings are not supported on iOS, since iOS does not have a Java runtime.

OpenSSL
^^^^^^^

OpenSSL is required for OpenDDS Security.
To configure and build OpenSSL for iPhone targets, use the ``ios64-cross`` configuration flag, and set the following environment variables:

.. code:: shell

  export CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
  export CROSS_TOP=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer
  export CROSS_SDK=iPhoneOS.sdk
  export PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin:$PATH"

Run the configure script:

.. code:: shell

  ./Configure ios64-cross no-shared no-dso no-hw no-engine --prefix=/usr/local/ios/openssl

Build with make:

.. code:: shell

  make

Install with make:

.. code:: shell

  make install

Set ``OPENSSL_ROOT`` to the install location:

.. code:: shell

  export OPENSSL_ROOT=/usr/local/ios/openssl

Note that the directory given by ``--prefix=`` will be created by ``make install`` and will have ``include`` and ``lib`` subdirectories that will be used by the OpenSSL build.

OpenSSL builds for iOS simulators are not as well-supported, and require a minor modification to the generated makefile.
To build OpenSSL for iOS simulators, set the following environment variables:

.. code:: shell

  export CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
  export CROSS_TOP=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer
  export CROSS_SDK=iPhoneSimulator.sdk
  export PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin:$PATH"

Run the configure script in ``openssl-1.1.1d/``:

.. code:: shell

  ./Configure darwin64-x86_64-cc no-shared no-dso no-hw no-engine --prefix=/usr/local/ios/openssl

Modify the generated Makefile setting ``CNF_CFLAGS``, and ``CNF_CXXFLAGS`` to:

.. code:: shell

  CNF_CFLAGS=-arch x86_64 -miphoneos-version-min=12.0 -isysroot $(CROSS_TOP)/SDKs/$(CROSS_SDK)

Build with make:

.. code:: shell

  make

Install with make:

.. code:: shell

  make install

Set ``OPENSSL_ROOT`` to the install location:

.. code:: shell

  export OPENSSL_ROOT=/usr/local/ios/openssl

Note that the directory given by ``--prefix=`` will be created by ``make install`` and will have ``include`` and ``lib`` subdirectories that will be used by the OpenSSL build.

OpenDDS security builds need the location of OpenSSL installed headers and static libraries.
This location can be passed to the OpenDDS configure script with the ``--openssl=\${OPENSSL_ROOT}`` flag.

Xerces
^^^^^^

Xerces C++ is also required for OpenDDS Security.
Xerces builds for iOS use CMake and require passing the cross-compile flags needed by the macOS C and C++ compilers as well as setting the iOS SDK root and other build flags.
All setting can be passed on the command line.

A representative simulator build can be configured as:

.. code:: shell

  CC="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang -miphoneos-version-min=12.0 -arch x86_64" \
  CXX="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -miphoneos-version-min=12.0 -arch x86_64" \
  cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -Dnetwork:BOOL=OFF -Dtranscoder=iconv \
    -DCMAKE_INSTALL_PREFIX=/usr/local/ios/xerces3 \
    -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"

After configuring the build, run CMake:

.. code:: shell

  cmake --build src

Install with make:

.. code:: shell

  make install

Set ``$XERCES_ROOT`` to the install location:

.. code:: shell

  export XERCES_ROOT=/usr/local/ios/xerces3

Note that the directory given by ``--prefix=`` will be created by ``make install`` and will have ``include`` and ``lib`` subdirectories that will be used by the Xerces build.

Configuring Xerces for a iPhone hardware build is similar to the simulator build.
The ``--arch`` flag changes to ``arm64`` and the ``CMAKE_OS_SYSROOT`` changes to the location of the iPhone SDK.

.. code:: shell

  CC="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang -miphoneos-version-min=12.0 -arch arm64" \
  CXX="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -miphoneos-version-min=12.0 -arch arm64" \
  cmake . -DBUILD_SHARED_LIBS:BOOL=OFF -Dnetwork:BOOL=OFF -Dtranscoder=iconv \
    -DCMAKE_INSTALL_PREFIX=/usr/local/ios/xerces3 \
    -DCMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"

OpenDDS security builds need the location of Xerces installed headers and static libraries.
This location can be passed to the OpenDDS configure script with the ``--xerces3=${XERCES_ROOT}`` flag.

*****************************
Cross-Compiling IDL Libraries
*****************************

Like all OpenDDS applications, you will need to use type support libraries generated from IDL files to use most of OpenDDS’s functionality.

Assuming the library is already setup and works for a desktop platform, then you should be able to run the DDS environment setup script from the ``OpenDDS/builds/target`` directory to set the appropriate iOS build flags:

.. code:: shell

  (source $DDS_ROOT/setenv.sh; mwc.pl -type gnuace . && make)

The resulting native IDL library file must be included with the rest of the native library files.

.. _ios-using-opendds-in-a-ios-app:

***************************
Using OpenDDS in an iOS App
***************************

Copy the static libraries in ``$ACE_ROOT/lib``, ``$DDS_ROOT/lib``, and the cross-compiled IDL libraries (and optionally ``$OPENSSL_ROOT/lib`` and ``$XERCES_ROOT/lib``) to the Xcode project or framework directory.

In the Xcode project, add ``$ACE_ROOT`` and ``$DDS_ROOT`` the header search paths.
