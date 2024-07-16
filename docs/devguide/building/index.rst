.. _building:

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

The OpenDDS Foundation regularly builds and tests OpenDDS on a wide variety of platforms, operating systems, and compilers.
The OpenDDS Foundation continually updates OpenDDS to support additional platforms.
See the :ghfile:`README.md#supported-platforms` file in the distribution for the most recent platform support information.
See :ref:`cross_compiling` for how to cross compile for other platforms.

************************
Configuring and Building
************************

.. seealso:: :ref:`cmake-building` to use CMake instead of a :term:`MPC`-based build.

.. ifconfig:: is_release

  If not already done, download the source from :ghrelease:`GitHub`.

Use the :ghfile:`configure` script to prepare to build OpenDDS.
This script requires :ref:`deps-perl`.

.. tab:: Linux, macOS, BSDs, etc.

  To start the script, change to the root of the OpenDDS source directory and run:

  .. code-block:: bash

    ./configure

.. tab:: Windows

  `Strawberry Perl <https://strawberryperl.com>`__ is recommended for Windows.

  To start the script, open a `Visual Studio Native Tools Command Prompt <https://learn.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line>`__
  or `Developer Command Prompt <https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell>`__ that has C++ tools available, then change to the root of the OpenDDS source directory and run:

  .. code-block:: batch

    configure

Optionally add ``--help`` to the command line to see the advanced options available for this script.
The configure script will download :ref:`ACE/TAO <deps-ace-tao>` and configure it for your platform.
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
  Before building, check that the "Configuration" and "Platform" are correct.
  In Visual Studio, select "Build" and then "Build Solution".

  It can also be built directly from the command prompt by using MSBuild.
  For example, if the configure script was ran without any arguments, to do a Debug x64 build:

  .. code-block:: batch

    msbuild -p:Configuration=Debug,Platform=x64 -m DDS_TAOv2.sln

  .. seealso:: `Microsoft MSBuild Documentation <https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild>`__

  The configure script creates an environment setup file called ``setenv.cmd`` that sets all the environment variables the build and test steps rely on.
  For the command prompt that ran the configure script, these variables were set automatically.
  If running in another command prompt, the variables need to be set again before building other projects and running tests:

  .. code-block::

    setenv

Java
====

If you're building OpenDDS for use by :ref:`Java applications <java>`, please see the file :ghfile:`java/INSTALL` instead of this one.

.. _building-sec:

Security
========

..
    Sect<14.1>

:ref:`sec` is disabled by default, and must be enabled by passing ``--security`` to the configure script.
It requires :ref:`deps-xerces` and :ref:`deps-openssl`.

.. tab:: Linux, macOS, BSDs, etc.

  .. tab:: Installed from a Package Manager

    Most package managers should have Xerces and OpenSSL packages that can be installed if not already:

    - Debian/Ubuntu-based: ``sudo apt install libxerces-c-dev libssl-dev``
    - Redhat/Fedora-based: ``sudo yum install xerces-c-devel openssl-devel``
    - Homebrew ``brew install xerces-c openssl@3``

  .. tab:: Built from Source

    Download source and build according to instructions.

  If the libraries didn't get installed to ``/usr``, then the installation prefixes will have to be passed using ``--xerces`` and ``--openssl``.

.. tab:: Windows

  .. tab:: Installed from a Package Manager

    **Using Microsoft vcpkg**

    Microsoft vcpkg is a "C++ Library Manager for Windows, Linux, and macOS" which helps developers build/install dependencies.
    Although it is cross-platform, this guide only discusses vcpkg on Windows.

    As of this writing, vcpkg is only supported on Visual Studio 2015 Update 3 and later versions; if using an earlier version of Visual Studio, skip down to the manual setup instructions later in this section.

    * If OpenDDS tests will be built, install CMake or put the one that comes with Visual Studio on the ``PATH`` (see ``Common7\IDE\CommonExtensions\Microsoft\CMake``).

    * If you need to obtain and install vcpkg, navigate to `https://github.com/Microsoft/vcpkg <https://github.com/Microsoft/vcpkg>`__ and follow the instructions to obtain vcpkg by cloning the repository and bootstrapping it.

    * Fetch and build the dependencies; by default, vcpkg targets x86 so be sure to specify the x64 target if required by specifying it when invoking vcpkg install, as shown here:

      .. code-block:: batch

          vcpkg install openssl:x64-windows xerces-c:x64-windows

    * Configure OpenDDS by passing the ``--openssl`` and ``--xerces3`` options.
      As a convenience, it can be helpful to set an environment variable to store the path since it is the same location for both dependencies.

      .. code-block:: batch

          set VCPKG_INSTALL=c:\path\to\vcpkg\installed\x64-windows
          configure --security --openssl=%VCPKG_INSTALL% --xerces3=%VCPKG_INSTALL%

    * Compile with ``msbuild``:

      .. code-block:: batch

          msbuild /m DDS_TAOv2_all.sln

      Or by launching Visual Studio from this command prompt so it inherits the correct environment variables and building from there:

      .. code-block:: batch

          devenv DDS_TAOv2_all.sln

  .. tab:: Built from Source

    .. note::

       For all of the build steps listed here, check that each package targets the same architecture (either 32-bit or 64-bit) by compiling all dependencies within the same type of Developer Command Prompt.

    **Compiling OpenSSL**

    Official OpenSSL instructions can be found `here <https://wiki.openssl.org/index.php/Compilation_and_Installation#Windows>`__.

    #. Install :ref:`deps-perl` and add it to the ``PATH`` environment variable.

    #. Install Netwide Assembler (NASM).
       Click through the latest stable release and there is a win32 and win64 directory containing executable installers.
       The installer does not update the Path environment variable, so a manual entry (``%LOCALAPPDATA%\bin\NASM``) is necessary.

    #. Download the required version of OpenSSL by cloning the repository.

    #. Open a Developer Command Prompt (32-bit or 64-bit depending on the desired target architecture) and change into the freshly cloned openssl directory.

    #. Run the configure script and specify a required architecture (``perl Configure VC-WIN32`` or ``perl Configure VC-WIN64A``).

    #. Run ``nmake``

    #. Run ``nmake install``

    .. note::

       If the default OpenSSL location is desired, which will be searched by OpenDDS, open the "Developer Command Prompt" as an administrator before running the install.
       It will write to ``C:\Program Files`` or ``C:\Program Files (x86)`` depending on the architecture.

    **Compiling Xerces-C++ 3**

    Official Xerces instructions can be found `here <https://xerces.apache.org/xerces-c/build-3.html>`__.

    #. Download/extract the Xerces source files.

    #. Create a cmake build directory and change into it (from within the Xerces source tree).

       .. code-block:: batch

           mkdir build
           cd build

    #. Run cmake with the appropriate generator.
       In this case Visual Studio 2017 with 64-bit is being used so:

       .. code-block:: batch

           cmake -G "Visual Studio 15 2017 Win64" ..

    #. Run cmake again with the build switch and install target (this should be done in an administrator command-prompt to install in the default location as mentioned above).

       .. code-block:: batch

           cmake --build . --target install

    **Configuring and Building OpenDDS**:

    #. Change into the OpenDDS root folder and run configure with security enabled.

       * If the default location was used for OpenSSL and Xerces, configure should automatically find the dependencies:

         .. code-block:: batch

             configure --security

    #. If a different location was used (assuming environment variables ``NEW_SSL_ROOT`` and ``NEW_XERCES_ROOT`` point to their respective library directories):

       .. code-block:: batch

           configure --security --openssl=%NEW_SSL_ROOT% --xerces3=%NEW_XERCES_ROOT%

    #. Compile with msbuild (or by opening the solution file in Visual Studio and building from there).

       .. code-block:: batch

           msbuild /m DDS_TAOv2_all.sln

Optional Features
=================

To avoid compiling OpenDDS code that you will not be using, there are certain features than can be excluded from being built.
The features are discussed below.

Users requiring a small-footprint configuration or compatibility with safety-oriented platforms should consider using the OpenDDS Safety Profile, which is described in :ref:`safety_profile` of this guide.

.. _building--building-with-a-feature-enabled-or-disabled:

Building With a Feature Enabled or Disabled
-------------------------------------------

..
    Sect<1.3.1>

Most features are supported by the ``configure`` script.
The ``configure`` script creates config files with the correct content and then runs MPC.
If you are using the ``configure`` script, run it with the ``--help`` command line option and look for the feature you wish to enable/disable.
If you are not using the ``configure`` script, continue reading below for instructions on running MPC directly.

For the features described below, MPC is used for enabling (the default) a feature or disabling the feature.
For a feature named *feature*, the following steps are used to disable the feature from the build:

#. Use the command line ``features`` argument to MPC:

   .. code-block:: bash

      mwc.pl -type type -features feature=0 DDS.mwc

   Or alternatively, add the line ``feature=0`` to the file ``$ACE_ROOT/bin/MakeProjectCreator/config/default.features`` and regenerate the project files using MPC.

#. If you are using the ``gnuace`` MPC project type (which is the case if you will be using GNU make as your build system), add line ``feature=0`` to the file ``$ACE_ROOT/include/makeinclude/platform_macros.GNU``.

To explicitly enable the feature, use ``feature=1`` above.

.. note:: You can also use the :ghfile:`configure` script to enable or disable features.
  To disable the feature, pass ``--no-feature`` to the script, to enable pass ``--feature``.
  In this case ``-`` is used instead of ``_`` in the feature name.
  For example, to disable feature ``content_subscription`` discussed below, pass ``--no-content-subscription`` to the configure script.

.. _building--disabling-the-building-of-built-in-topic-support:

Disabling the Building of Built-in Topic Support
------------------------------------------------

..
    Sect<1.3.2>

Feature Name: ``built_in_topics``

You can reduce the footprint of the core DDS library by up to 30% by disabling Built-in Topic Support.
See :ref:`bit` for a description of built-in topics.

.. _building--disabling-the-building-of-compliance-profile-features:

Disabling the Building of Compliance Profile Features
-----------------------------------------------------

..
    Sect<1.3.3>

The DDS specification defines *compliance profiles* to provide a common terminology for indicating certain feature sets that a DDS implementation may or may not support.
These profiles are given below, along with the name of the MPC feature to use to disable support for that profile or components of that profile.

Many of the profile options involve QoS settings.
If you attempt to use a QoS value that is incompatible with a disabled profile, a runtime error will occur.
If a profile involves a class, a compile time error will occur if you try to use the class and the profile is disabled.

.. _building--content-subscription-profile:

Content-Subscription Profile
============================

..
    Sect<1.3.3.1>

Feature Name: ``content_subscription``

This profile adds the classes ``ContentFilteredTopic``, ``QueryCondition``, and ``MultiTopic`` discussed in :ref:`content_subscription_profile`.

In addition, individual classes can be excluded by using the features given in the table below.

.. list-table:: Content-Subscription Class Features
   :header-rows: 1

   * - Class

     - Feature

   * - ContentFilteredTopic

     - ``content_filtered_topic``

   * - QueryCondition

     - ``query_condition``

   * - MultiTopic

     - ``multi_topic``

.. _building--persistence-profile:

Persistence Profile
===================

..
    Sect<1.3.3.2>

Feature Name: ``persistence_profile``

This profile adds the :ref:`qos-durability-service` policy and the settings ``TRANSIENT`` and ``PERSISTENT`` of the :ref:`qos-durability` policy ``kind``.

.. _building--ownership-profile:

Ownership Profile
=================

..
    Sect<1.3.3.3>

Feature Name: ``ownership_profile``

This profile adds:

* the setting ``EXCLUSIVE`` of :ref:`qos-ownership`

* support for the :ref:`qos-ownership-strength` policy

* setting a ``depth > 1`` for the :ref:`qos-history` policy

Some users may wish to exclude support for the exclusive :ref:`qos-ownership` policy and its associated :ref:`qos-ownership-strength` without impacting use of :ref:`qos-history`.
In order to support this configuration, OpenDDS also has the MPC feature ``ownership_kind_exclusive`` (configure script option ``--no-ownership-kind-exclusive``).

.. _building--object-model-profile:

Object Model Profile
=====================

..
    Sect<1.3.3.4>

Feature Name: ``object_model_profile``

This profile includes support for the :ref:`qos-presentation` ``access_scope`` setting of ``GROUP``.

.. note:: Currently, the :ref:`qos-presentation` ``access_scope`` of ``TOPIC`` is also excluded when ``object_model_profile`` is disabled.

.. _cross_compiling:

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

.. _building--building-applications-that-use-opendds:

**************************************
Building Applications that use OpenDDS
**************************************

..
    Sect<1.4>

This section applies to any C++ code that directly or indirectly includes OpenDDS headers.
For Java applications, see :ref:`java`.

C++ source code that includes OpenDDS headers can be built using either build system: MPC or CMake.

.. _building--mpc-the-makefile-project-and-workspace-creator:

MPC: The Makefile, Project, and Workspace Creator
=================================================

..
    Sect<1.4.1>

OpenDDS is itself built with MPC, so development systems that are set up to use OpenDDS already have MPC available.
The OpenDDS configure script creates a "setenv" script with environment settings (``setenv.cmd`` on Windows; ``setenv.sh`` on Linux/macOS).
This environment contains the ``PATH`` and ``MPC_ROOT`` settings necessary to use MPC.

MPC's source tree (in ``MPC_ROOT``) contains a ``docs`` directory with both HTML and plain text documentation (``USAGE`` and ``README`` files).

The example walk-through in :ref:`getting_started--using-dcps` uses MPC as its build system.
The OpenDDS source tree contains many tests and examples that are built with MPC.
These can be used as starting points for application MPC files.

.. _building--cmake:

CMake
=====

..
    Sect<1.4.2>

Applications can also be built with `CMake <https://cmake.org/>`__.
See :doc:`cmake` for more information.

.. _building--custom-build-systems:

Custom Build systems
====================

..
    Sect<1.4.3>

Users of OpenDDS are strongly encouraged to select one of the two options listed above (MPC or CMake) to generate consistent build files on any supported platform.
If this is not possible, users of OpenDDS must make sure that all code generator, compiler, and linker settings in the custom build setup result in API- and ABI-compatible code.
To do this, start with an MPC or CMake-generated project file (makefile or Visual Studio project file) and make sure all relevant settings are represented in the custom build system.
This is often done through a combination of inspecting the project file and running the build with verbose output to see how the toolchain (code generators, compiler, linker) is invoked.

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

.. _cmake-building-ace-tao:

ACE/TAO
^^^^^^^

A prebuilt ACE/TAO can be passed using :cmake:var:`OPENDDS_ACE`.
In that case :ref:`cmake-feature-vars` will be automatically derived from ACE's ``default.features`` file.
If :cmake:var:`OPENDDS_ACE` is not passed, then ACE/TAO will be built.
When building ACE/TAO a release is downloaded by default, but source can also be provided using :cmake:var:`OPENDDS_ACE_TAO_SRC` or cloned using :cmake:var:`OPENDDS_ACE_TAO_GIT`.
:cmake:var:`OPENDDS_ACE_TAO_KIND` controls what version of ACE/TAO is downloaded for both releases and :cmake:var:`OPENDDS_ACE_TAO_GIT`.

.. _cmake-building-vars:

Build-Exclusive CMake Variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These are all the variables that are exclusive to building OpenDDS with CMake:

.. cmake:var:: OPENDDS_JUST_BUILD_HOST_TOOLS

  If true, ``opendds_idl`` is the only thing built for OpenDDS.
  If ACE/TAO is also being built, then ``ace_gperf``, ``tao_idl``, and their dependencies are also built.
  The build directory for this can be passed to :cmake:var:`OPENDDS_HOST_TOOLS`.

.. cmake:var:: OPENDDS_ACE_TAO_SRC

  If defined, sets the ACE/TAO to build and use.
  A prebuilt ACE/TAO can be provided using :cmake:var:`OPENDDS_ACE`.
  By default, a hardcoded release depending on :cmake:var:`OPENDDS_ACE_TAO_KIND` is downloaded.

.. cmake:var:: OPENDDS_ACE_TAO_KIND

  The default is ``ace7tao3`` for :ref:`ACE 7/TAO 3 <ace7tao3>`.
  See :ref:`here <deps-ace-tao>` for other versions of ACE/TAO.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_ACE_TAO_GIT

  Implies :cmake:var:`OPENDDS_MPC_GIT`.
  If true clone ACE/TAO from :cmake:var:`OPENDDS_ACE_TAO_GIT_TAG` at :cmake:var:`OPENDDS_ACE_TAO_GIT_REPO`.
  By default, a hardcoded release depending on :cmake:var:`OPENDDS_ACE_TAO_KIND` is downloaded.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_ACE_TAO_GIT_REPO

  Implies :cmake:var:`OPENDDS_ACE_TAO_GIT`.
  The Git repository to clone ACE/TAO from.
  The default is ``https://github.com/DOCGroup/ACE_TAO``.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_ACE_TAO_GIT_TAG

  Implies :cmake:var:`OPENDDS_ACE_TAO_GIT`.
  The Git tag to clone ACE/TAO from.
  The default depends on :cmake:var:`OPENDDS_ACE_TAO_KIND`.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_MPC

  Path to :ref:`deps-mpc`.
  In most cases this will be provided and automatically detected, unless ACE/TAO was cloned manually and provided using :cmake:var:`OPENDDS_ACE_TAO_SRC`.

  .. versionadded:: 3.26, but documented in 3.27

.. cmake:var:: OPENDDS_MPC_GIT

  If true clone MPC from :cmake:var:`OPENDDS_MPC_GIT_TAG` at :cmake:var:`OPENDDS_MPC_GIT_REPO`.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_MPC_GIT_REPO

  Implies :cmake:var:`OPENDDS_MPC_GIT`.
  The Git repository to clone MPC from.
  The default is ``https://github.com/DOCGroup/MPC``.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_MPC_GIT_TAG

  Implies :cmake:var:`OPENDDS_MPC_GIT`.
  This is the Git tag to clone MPC from.
  The default is ``master``.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_BUILD_TESTS

  Build tests that are currently supported when building OpenDDS with CMake.
  See :ref:`cmake-running-tests` for how to run them.
  The default for this is ``BUILD_TESTING`` (usually false).

.. cmake:var:: OPENDDS_BUILD_EXAMPLES

  Build examples that are currently supported when building OpenDDS with CMake.
  See :ref:`cmake-running-tests` for how to run them.
  The default for this is ``TRUE``.

.. cmake:var:: OPENDDS_BOOTTIME_TIMERS
  :no-contents-entry:

  .. versionadded:: 3.28

  OpenDDS uses CLOCK_BOOTTIME when scheduling timers.
  On some platforms the default is to use CLOCK_MONOTONIC which does not increment when the system is suspended.
  Enable this option to use CLOCK_BOOTTIME as the timer base clock instead of CLOCK_MONOTONIC.
  Default is ``OFF``.

.. cmake:var:: OPENDDS_COMPILE_WARNINGS

  If set to ``WARNING``, enables additional compiler warnings when compiling OpenDDS.
  If set to ``ERROR``, enables additional compiler warnings which are treated as errors when compiling OpenDDS.

  .. versionadded:: 3.28

.. _cmake-building-speed:

Speeding up the Build
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
- The host tools will build its own ACE/TAO for the host system, but this can be overridden using :cmake:var:`OPENDDS_ACE` on the host build and :cmake:var:`OPENDDS_ACE_TAO_HOST_TOOLS` on the target build.
- If the target platform isn't automatically supported, then ACE/TAO will have to be :ref:`built separately <cmake-ace-tao-manual>`.

Android
^^^^^^^

The following is an example of cross-compiling OpenDDS for Android on Linux using CMake.
It assumes the NDK has been downloaded and the location is in an environment variable called ``NDK``.

.. code-block:: shell

  # Build Host Tools
  cmake -B build-host \
    -DBUILD_SHARED_LIBS=FALSE \
    -DOPENDDS_JUST_BUILD_HOST_TOOLS=TRUE
  cmake --build build-host -- -j 4

  # Build OpenDDS for Android
  cmake -B build-target \
    -DBUILD_SHARED_LIBS=TRUE \
    -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-29 \
    --toolchain $NDK/build/cmake/android.toolchain.cmake \
    -DOPENDDS_HOST_TOOLS=$(realpath build-host)
  cmake --build build-target-- -j 4

Installation
============

Once built, OpenDDS can be installed using `cmake --install <https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project>`__.
Currently ACE/TAO has to be installed separately and this is only possible with GNU Make.

Using a CMake-built OpenDDS
===========================

After building and optionally installing OpenDDS, it can be used through the same :doc:`CMake package <cmake>` as an MPC-built OpenDDS.

.. _cmake-running-tests:

Running Tests
=============

:cmake:var:`Tests <OPENDDS_BUILD_TESTS>` and :cmake:var:`examples <OPENDDS_BUILD_EXAMPLES>` can be run using using `ctest <https://cmake.org/cmake/help/latest/manual/ctest.1.html>`__.
There is also a target for running tests in the build, but the name differs based on the CMake generator used.

.. _cmake-known-limitations:

Known Limitations
=================

.. _cmake-ace-tao-manual:

- ACE/TAO can't be automatically built unless there is explicit support for the platform.
  Currently this only exists for Windows, Linux, macOS, and Android.
  All other platforms will require configuring and building ACE/TAO separately and passing the path using :cmake:var:`OPENDDS_ACE`.

  - See https://www.dre.vanderbilt.edu/~schmidt/DOC_ROOT/ACE/ACE-INSTALL.html for how to manually build ACE/TAO.

- The following features are planned, but not implemented yet:

  - Support for :ref:`safety_profile`
  - Support for :ref:`java`
  - The ability to use MPC for building user applications with an installed CMake-built OpenDDS
