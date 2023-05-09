#######################
Building and Installing
#######################

.. toctree::
  :hidden:

  dependencies

Except for an `example Docker container <https://github.com/OpenDDS/OpenDDS/pkgs/container/opendds>`__, there are no official prebuilt packages of OpenDDS, so it to be built from source.

.. ifconfig:: is_release

  If not done already, download the source from :ghrelease:`GitHub`.

*******************
Supported Platforms
*******************

We have built OpenDDS on number of different platforms and compilers.
See :ghfile:`README.md#supported-platforms` for a complete description of supported platforms.

************************
Configuring and Building
************************

Use the :ghfile:`configure` script to prepare to build OpenDDS.
This script requires Perl 5.18 or newer to be installed and available on the system ``PATH``.
`Strawberry Perl <https://www.strawberryperl.com>`__ is recommended on Windows.
To start the script simply change to the root of the OpenDDS source directory and run:

.. tab:: Linux, macOS, BSDs, etc.

  .. code-block::

    ./configure

.. tab:: Windows

  .. code-block::

    configure

Optionally add ``--help`` to the command line to see the advanced options available for this script.
The configure script will download ACE/TAO and configure it for your platform.
To use an existing ACE/TAO installation, either set the :envvar:`ACE_ROOT` and :envvar:`TAO_ROOT` environment variables or pass the ``--ace`` and ``--tao`` (if TAO is not at ``$ACE_ROOT/TAO``) options to configure.
If configure runs successfully it will end with a message about the next steps for compiling OpenDDS.

OpenDDS supports parallel builds to speed up the build when using Make.
To use this pass ``-j N`` where ``N`` the max number of parallel jobs to run.
If not sure what ``N`` should be, use the number of cores on the machine.

The configure script creates an environment setup file called ``setenv`` (actually named ``setenv.sh`` or ``setenv.cmd`` depending on platform) that restores all the environment variables the build and test steps rely on.
The main makefile for non-Windows builds temporarily sets the environment as well, so ``setenv.sh`` is not needed when running ``make`` from the top level.
On Windows, the configure script modifies the environment of the command prompt that ran it.
If using a new environment, use ``setenv.cmd`` to set the required environment variables before running Visual Studio.

Java
====

If you're building OpenDDS for use by Java applications, please see the file :ghfile:`java/INSTALL` instead of this one.

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

The instructions for building for the Raspberry Pi are on `opendds.org <http://opendds.org/quickstart/GettingStartedPi.html>`__.

Android
-------

Android support is documented in :ghfile:`docs/android.md`.

Apple iOS
---------

Apple iOS support is documented in :ghfile:`docs/ios.md`.

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
To use CMake to generate application makefiles, see :ghfile:`docs/cmake.md`.

*****
Tests
*****

Tests are not built by default, ``--tests`` must be passed to the configure script.
All tests can be run using :ghfile:`tests/auto_run_tests.pl`.
See :doc:`/internal/running_tests` for running all tests or inividual tests.
