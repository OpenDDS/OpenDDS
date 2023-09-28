#######################
Building and Installing
#######################

.. toctree::
  :hidden:

  dependencies
  cmake
  android
  ios

*******************
Supported Platforms
*******************

We have built OpenDDS on number of different platforms and compilers.
See :ghfile:`README.md#supported-platforms` for a complete description of supported platforms.
See :ref:`cross-compiling` for how to cross compile for other platforms.

************************
Configuring and Building
************************

.. seealso:: :ref:`cmake-building` to use CMake instead of a :term:`MPC`-based build.

.. ifconfig:: is_release

  If not already done, download the source from :ghrelease:`GitHub`.

Use the :ghfile:`configure` script to prepare to build OpenDDS.
This script requires Perl 5.18 or newer to be installed and available on the system ``PATH``.
Older versions of Perl will probably work, but are not tested.

.. tab:: Linux, macOS, BSDs, etc.

  To start the script change to the root of the OpenDDS source directory and run:

  .. code-block::

    ./configure

.. tab:: Windows

  `Strawberry Perl <https://strawberryperl.com>`__ is recommended for Windows.

  To start the script open a `Visual Studio Developer Command Prompt <https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell>`__ that has C++ tools available, then change to the root of the OpenDDS source directory and run:

  .. code-block::

    configure

Optionally add ``--help`` to the command line to see the advanced options available for this script.
The configure script will download ACE/TAO and configure it for your platform.
To use an existing ACE/TAO installation, either set the :envvar:`ACE_ROOT` and :envvar:`TAO_ROOT` environment variables or pass the ``--ace`` and ``--tao`` (if TAO is not at ``$ACE_ROOT/TAO``) options to configure.

.. seealso:: :doc:`dependencies` for a full list of dependencies including ones that can be configured with the configure script.

If configure runs successfully it will end with a message about the next steps for compiling OpenDDS.

.. tab:: Linux, macOS, BSDs, etc.

  OpenDDS on these platforms must be built using GNU Make:

  .. code-block::

    make -j 4

  OpenDDS supports parallel builds to speed up the build when using Make.
  Above 4 is used as an example for the max number of parallel jobs.
  If unsure what this number should be, use the number of CPU cores on the machine.

  The configure script creates an environment setup file called ``setenv.sh`` that sets all the environment variables the build and test steps rely on.
  The main makefile sets these itself, so ``setenv.sh`` is not needed when running ``make`` from the top level.
  It needs to be sourced before building other projects and running tests:

  .. code-block:: shell

    source setenv.sh

.. tab:: Windows

  The configure script will say how to open the solution file for OpenDDS in Visual Studio using ``devenv``.

  It can also be built directly from the command prompt by using MSBuild.
  For example, if the configure script was ran without any arguments, to do a Debug x64 build:

  .. code-block:: batch

    msbuild -p:Configuration=Debug,Platform=x64 -m DDS_TAOv2.sln

  .. seealso:: `Microsoft MSBuild Documentation <https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild>`__

  The configure script creates an environment setup file called ``setenv.sh`` that sets all the environment variables the build and test steps rely on.
  For the command prompt that ran the configure script, these variables were set automatically.
  If running in another command prompt, the variables need to be set again before building other projects and running tests:

  .. code-block::

    setenv.cmd

Java
====

If you're building OpenDDS for use by Java applications, please see the file :ghfile:`java/INSTALL` instead of this one.

.. _cross-compiling:

Cross Compiling
===============

Use the configure script, and set the target platform to one different than the host.
For example:

.. code-block:: shell

  ./configure --target=lynxos-178

Run configure with ``--target-help`` for details on the supported targets.
In this setup, configure will clone the OpenDDS and ACE/TAO source trees for host and target builds.
It will do a static build of the host tools (such as ``opendds_idl`` and ``tao_idl``) in the host environment, and a full build in the target environment.
Most parameters to configure are then assumed to be target parameters.

Any testing has to be done manually.

Raspberry Pi
------------

The instructions for building for the Raspberry Pi are on `opendds.org <https://opendds.org/quickstart/GettingStartedPi.html>`__.

Android
-------

Android support is documented in :doc:`android`.

Apple iOS
---------

Apple iOS support is documented in :doc:`ios`.

.. _install:

************
Installation
************

When OpenDDS is built using ``make``, if the configure script was run with an argument of ``--prefix=<prefix>`` the ``make install`` target is available.

After running ``make`` (and before ``make install``) you have one completely ready and usable OpenDDS.
Its ``DDS_ROOT`` is the top of the source tree -- the same directory from which you ran configure and make.
That ``DDS_ROOT`` should work for building application code, and some users may prefer using it this way.

After ``make install`` there is a second completely ready and usable OpenDDS that's under the installation prefix directory.
It contains the required libraries, code generators, header files, IDL files, and associated scripts and documentation.

.. note:: If configured with RapidJSON, OpenDDS will install the headers for RapidJSON, which might conflict with an existing installation.

Using an Installed OpenDDS
==========================

After ``make install`` completes, the shell script in ``<prefix>/share/dds/dds-devel.sh`` is used to set the ``DDS_ROOT`` environment variable.
The analogous files for ACE and TAO are ``<prefix>/share/ace/ace-devel.sh`` and ``<prefix>/share/tao/tao-devel.sh``.

The ``<prefix>`` tree does not contain a tool for makefile generation.
To use MPC to generate application makefiles, the ``MPC_ROOT`` subdirectory from the OpenDDS source tree can be used either in-place or copied elsewhere.
To use CMake to generate application makefiles, see :doc:`cmake`.

*****
Tests
*****

Tests are not built by default, ``--tests`` must be passed to the configure script.
All tests can be run using :ghfile:`tests/auto_run_tests.pl`.
See :doc:`/internal/running_tests` for running all tests or individual tests.

.. _cmake-building:

****************************
Building OpenDDS Using CMake
****************************

.. versionadded:: 3.26

OpenDDS can be built with CMake 3.23 or later.

Configuring and Building
========================

.. ifconfig:: is_release

  If not already done, download the source from :ghrelease:`GitHub`.

To configure and build:

.. tab:: Linux, macOS, BSDs, etc.

  .. code-block::

    cmake -B build -DCMAKE_UNITY_BUILD=TRUE
    cmake --build build -- -j 4

  4 was used as an example for the max number of parallel jobs.
  If unsure what this number should be, use the number of CPU cores on the machine.
  This can be combined with unity builds.

.. tab:: Windows

  .. code-block::

    cmake -B build -DCMAKE_UNITY_BUILD=TRUE
    cmake --build build

Variables
---------

Unless otherwise noted, the build features and behavior can be controlled by the OpenDDS Config Package :ref:`cmake-config-vars`.
A prebuilt ACE/TAO can be passed using :cmake:var:`OPENDDS_ACE`.
In that case :ref:`cmake-feature-vars` will be automatically derived from ACE's ``default.features`` file.

These are the variables that are exclusive to building OpenDDS with CMake:

.. cmake:var:: OPENDDS_JUST_BUILD_HOST_TOOLS

  If true, just builds ``opendds_idl``.
  The build directory for this can be passed to :cmake:var:`OPENDDS_HOST_TOOLS`.

.. cmake:var:: OPENDDS_ACE_TAO_SRC

  If defined, sets the ACE/TAO to build and use.
  By default, a hardcoded release is downloaded.

.. cmake:var:: OPENDDS_BUILD_TESTS

  Build tests that are currently supported when building OpenDDS with CMake.
  See :ref:`cmake-running-tests` for how to run them.
  The default for this is ``BUILD_TESTING`` (usually false).

.. cmake:var:: OPENDDS_BUILD_EXAMPLES

  Build examples that are currently supported when building OpenDDS with CMake.
  See :ref:`cmake-running-tests` for how to run them.
  The default for this is :cmake:var:`OPENDDS_BUILD_TESTS`.

Speeding up the build
---------------------

A major speed up supported by all the CMake generators are `unity builds <https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html>`__.
This makes it so that multiple C++ source files are compiled at the same time by a compiler process.
This can be enabled by passing ``-DCMAKE_UNITY_BUILD=TRUE`` to the CMake configure command as shown in the example.
If there are problems with building, e.g. redefinition errors, then pass ``-DCMAKE_UNITY_BUILD=FALSE`` to override the cache variable in an existing build directory and disable unity builds.
Fresh build directories default to ``CMAKE_UNITY_BUILD=FALSE``.

The `Ninja <https://ninja-build.org/>`__ CMake generator can also be used to speed up builds as Ninja was built from scratch for parallel building and build systems like CMake.
If Ninja is available, pass ``-G Ninja`` to have CMake use it.
Building ACE/TAO with Ninja requires CMake 3.24 or later.
If building ACE/TAO, the CMake build will still use either Visual Studio or GNU Make internally to build ACE/TAO because MPC doesn't support Ninja.

Cross Compiling
---------------

Once set up properly, OpenDDS can be cross-compiled with CMake using normal `CMake cross compiling <https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html>`__.
A few things to note:

- Native-built host tools, like :term:`opendds_idl`, have to be configured and built separately and provided to the target build using :cmake:var:`OPENDDS_HOST_TOOLS`.
- The host tools can build its own ACE/TAO for the host system, but it will currently also build all of ACE/TAO.
  This can be skipped by providing a prebuilt ACE/TAO to the host tools build using :cmake:var:`OPENDDS_ACE`.
  This also has to be provided to the target build using :cmake:var:`OPENDDS_ACE_TAO_HOST_TOOLS`.
- ACE/TAO for the target build has to be configured and built separately and provided using :cmake:var:`OPENDDS_ACE`.

  - See https://www.dre.vanderbilt.edu/~schmidt/DOC_ROOT/ACE/ACE-INSTALL.html for how to manually build ACE/TAO.

The following is an example of cross-compiling OpenDDS for Android on Linux using CMake.
It assumes the NDK has been downloaded and the location is in an environment variables called ``NDK`` and the downloaded ACE/TAO version matches the version being used by host tools.

.. code-block:: shell

  # Build Host Tools
  cmake -B build-host -DOPENDDS_JUST_BUILD_HOST_TOOLS=TRUE
  cmake --build build-host -- -j 4
  export HOST_ROOT=$(realpath build-host/ace_tao)

  # Build ACE/TAO for Android
  curl -L https://github.com/DOCGroup/ACE_TAO/releases/download/ACE+TAO-7_1_0/ACE+TAO-7.1.0.tar.gz \
    --output ACE+TAO.tar.gz
  tar xzf ACE+TAO.tar.gz
  export ACE_ROOT=$(realpath ACE_wrappers)
  export TAO_ROOT=$ACE_ROOT/TAO
  export MPC_ROOT=$ACE_ROOT/MPC
  echo 'no_cxx11=0' > "$ACE_ROOT/bin/MakeProjectCreator/config/default.features"
  cat << EOF > "$ACE_ROOT/include/makeinclude/platform_macros.GNU"
  optimize = 0
  android_abi := armeabi-v7a
  android_api := 29
  android_ndk := \$(NDK)
  TAO_IDL := \$(HOST_ROOT)/bin/tao_idl
  TAO_IDLFLAGS += -g \$(HOST_ROOT)/ace_gperf
  TAO_IDL_DEP := \$(HOST_ROOT)/bin/tao_idl
  include \$(ACE_ROOT)/include/makeinclude/platform_android.GNU
  EOF
  cat << EOF > "$ACE_ROOT/ace/config.h"
  #define ACE_DISABLE_MKTEMP
  #define ACE_LACKS_READDIR_R
  #define ACE_LACKS_TEMPNAM
  #include "config-android.h"
  EOF
  cp ACE_TAO_for_OpenDDS.mwc $ACE_ROOT
  (cd $ACE_ROOT && bin/mwc.pl -type gnuace ACE_TAO_for_OpenDDS.mwc && make -j 4)

  # Build OpenDDS for Android
  cmake -B build-target \
    -DBUILD_SHARED_LIBS=TRUE \
    -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-29 \
    --toolchain $NDK/build/cmake/android.toolchain.cmake \
    -DOPENDDS_ACE_TAO_HOST_TOOLS=$(realpath build-host/ace_tao) \
    -DOPENDDS_HOST_TOOLS=$(realpath build-host) \
    -DOPENDDS_ACE=$ACE_ROOT
  cmake --build build-target -- -j 4

Installation
============

Once built, OpenDDS can be installed using `cmake --install <https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project>`__.
Currently ACE/TAO has to be installed separately and this is only possible with GNU Make.

.. _cmake-running-tests:

Running Tests
=============

Tests (:cmake:var:`OPENDDS_BUILD_TESTS`) and examples (:cmake:var:`OPENDDS_BUILD_EXAMPLES`) can be run using the ``test`` target (``cmake --build build -t test``) or directly using `ctest <https://cmake.org/cmake/help/latest/manual/ctest.1.html>`__.

Other Known Limitations
=======================

- The following features are planned, but not implemented yet:

  - The ability to use MPC for building user applications.
  - Safety profile
  - Java Mapping

- CMake-build OpenDDS libraries and executables will currently be ignored by :ref:`find_package(OpenDDS COMPONENTS ...) <cmake-components>`.
  Passing feature should work.
