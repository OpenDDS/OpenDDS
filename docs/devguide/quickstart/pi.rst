############
Raspberry Pi
############

The goal of this guide is help you download and compile OpenDDS for Linux on the `Raspberry Pi <https://www.raspberrypi.org/>`__ and run a simple example.

.. _pi-build-directions:

****************
Build Directions
****************

For this guide you need a Raspberry Pi set up with Raspbian Bullseye Linux (the Lite variant works) and connected to a local network.
You will also need a Linux host system (or virtual machine) connected to the same network where we will build OpenDDS.
We have tested with Ubuntu 20.04.3 LTS x86_64 and CentOS Stream 8 x86_64.
This guide assumes you know the address and user credentials of the Pi.
Perform these all these steps on the host computer:


#. Ensure that your host environment has:

   * a C++ compiler
   * GNU Make
   * Perl
   * CMake if building with DDS Security
   * Optional Java SDK 1.5 or later for Java binding support

#. Download and extract the latest tar.gz file from the `download site <https://github.com/OpenDDS/OpenDDS/releases/latest/>`__

#. Download and extract the `Linux GCC 10.2 cross-compiler toolchain <https://developer.arm.com/-/media/Files/downloads/gnu-a/10.2-2020.11/binrel/gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf.tar.xz?revision=d0b90559-3960-4e4b-9297-7ddbc3e52783&hash=6F50B04F08298881CA3596CE99E5ABB3925DEB24>`__ from the `ARM Developer website <https://developer.arm.com/>`__.
   Rename the resulting directory from ``gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf`` to ``cross-pi-gcc`` and move it to ``/opt``.

#. If building with :ref:`dds_security`, follow :ref:`the steps below <thirdparty>` to build OpenSSL and Xerces-C++ for the Pi.

#. Enter the ``OpenDDS-<version>`` directory

#. Run the following as a single command.

   .. code-block:: bash

      ./configure --target=linux-cross --target-compiler=/opt/cross-pi-gcc/bin/arm-none-linux-gnueabihf-g++ (additional options)

   Additional options:

   * If using DDS Security:

     .. code-block:: bash

        --security --no-tests --openssl=SSL_ROOT --xerces3=XERCESCROOT

     Replace ``SSL_ROOT`` and ``XERCESCROOT`` with the paths for your system (see :ref:`below <thirdparty>`).

     Note that the Google Test Framework is not supported in this configuration, therefore ``--no-tests`` is required when using ``--security``.

   * If using Java bindings:

     .. code-block:: bash

        --java

#. .. code-block:: bash

      make

See `Support <https://opendds.org/support.html>`__ if you encounter problems with configuration or building.

*************************
Copying OpenDDS to the Pi
*************************

There are multiple ways to do this, including using a flash drive.
We do not recommend copying the entire build tree, since it can be 2+ GB.
The following steps copy the OpenDDS runtime libraries, support scripts, and test executables over the network.

#. Leave the ``OpenDDS-<version>`` directory:

   .. code-block:: bash

      cd ..

#. .. code-block:: bash

      tar czhf OpenDDS-<version>.tar.gz OpenDDS-<version>/build/target/ACE_wrappers/lib OpenDDS-<version>/build/target/lib OpenDDS-<version>/build/target/bin OpenDDS-<version>/build/target/ACE_wrappers/bin/PerlACE OpenDDS-<version>/build/target/DevGuideExamples/DCPS/Messenger

#. .. code-block:: bash

      scp OpenDDS.tar.gz USER@ADDRESS:

   where ``USER`` and ``ADDRESS`` are the username and IP address of your Raspberry Pi.
   It will ask you for the password for the user on the Pi.

#. .. code-block:: bash

      ssh USER@ADDRESS

   to access the Pi, taking the same information as the previous command.

#. .. code-block:: bash

      tar xzf OpenDDSOpenDDS-<version>.tar.gz

*************************
Run the Messenger Example
*************************

#. While still in ssh on the Pi, enter the ``OpenDDS-<version>`` directory
#. .. code-block:: bash

      export DDS_ROOT="$PWD/build/target"

#. .. code-block:: bash

      export ACE_ROOT="$DDS_ROOT/ACE_wrappers"

#. .. code-block:: bash

      export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:"$ACE_ROOT/lib":"$DDS_ROOT/lib"

#. .. code-block:: bash

      export PATH=${PATH}:"$ACE_ROOT/bin":"$DDS_ROOT/bin"

#. For the C++ example:

   .. code-block:: bash

      cd $DDS_ROOT/DevGuideExamples/DCPS/Messenger

   For the Java example:

   .. code-block:: bash

      cd $DDS_ROOT/java/tests/messenger

#. .. code-block:: bash

      ./run_test.pl

The Messenger Example starts an InfoRepo, publisher, and subscriber.
The InfoRepo allows the publisher and subscriber to find each other.
Once the publisher finds the subscriber, it sends 10 messages to the subscriber and waits 30 seconds for the subscriber to acknowledge the messages.

**********
Next Steps
**********

See :ref:`getting_started` for a detailed explanation of the Messenger C++ Example or :ref:`java` for the Java Example.

.. _thirdparty:

******************************
Building Third-Party Libraries
******************************

=====
Setup
=====

#. Create and enter a directory to perform the build.
#. Set the ``BUILD_ROOT`` shell variable to the working directory.
#. This will be the parent directory for the source repos and "staged" installation directories for the cross-compiled software.

=======
OpenSSL
=======

#. In ``$BUILD_ROOT``, download and extract the `OpenSSL source archive <https://www.openssl-library.org/source/>`__, and change to that extracted directory.
   See :ref:`deps-openssl` for any version requirements for OpenSSL.

#. .. code-block:: bash

      ./Configure --cross-compile-prefix=/opt/cross-pi-gcc/bin/arm-none-linux-gnueabihf- linux-armv4

#. .. code-block:: bash

      make

#. .. code-block:: bash

      make install DESTDIR=$BUILD_ROOT/pi-openssl

==========
Xerces-C++
==========

#. In ``$BUILD_ROOT``, create the file ``PiToolchain.cmake`` with the contents:

   .. code-block:: bash

      set(CMAKE_SYSTEM_NAME Linux)
      set(CMAKE_SYSTEM_PROCESSOR arm)
      set(CMAKE_C_COMPILER /opt/cross-pi-gcc/bin/arm-none-linux-gnueabihf-gcc)
      set(CMAKE_CXX_COMPILER /opt/cross-pi-gcc/bin/arm-none-linux-gnueabihf-g++)
      set(CMAKE_FIND_ROOT_PATH /opt/cross-pi-gcc/arm-none-linux-gnueabihf)
      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
      set(THREADS_PTHREAD_ARG 2)</pre></li>

#. In ``$BUILD_ROOT``, download and extract the `Xerces-C++ source archive <https://xerces.apache.org/xerces-c/download.cgi>`__, and change to that extracted directory.
   See :ref:`deps-xerces` for any version requirements for Xerces.

#. .. code-block:: bash

      mkdir build-pi
      cd build-pi

#. .. code-block:: bash

      cmake -DCMAKE_TOOLCHAIN_FILE=$BUILD_ROOT/PiToolchain.cmake -DCMAKE_INSTALL_PREFIX=$BUILD_ROOT/pi-xerces ..

#. .. code-block:: bash

      make

#. .. code-block:: bash

      make install

========================
Using these with OpenDDS
========================

* For ``configure`` (see :ref:`above <pi-build-directions>`)

  * ``SSL_ROOT`` is ``$BUILD_ROOT/pi-openssl/usr/local``
  * ``XERCESCROOT`` is ``$BUILD_ROOT/pi-xerces``

* For runtime loading of shared objects

  * copy ``$BUILD_ROOT/pi-openssl/usr/local/lib/libcrypto.so.1.1`` to ``build/target/lib``
  * copy ``$BUILD_ROOT/pi-xerces/lib/libxerces-c-3.2.so`` to ``build/target/lib``
