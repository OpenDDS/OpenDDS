<img src="docs/logo.svg" alt="OpenDDS" width="200">

# OpenDDS

[![Build Status](https://travis-ci.org/objectcomputing/OpenDDS.svg?branch=master)](https://travis-ci.org/objectcomputing/OpenDDS)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/opendds/badge.svg)](https://scan.coverity.com/projects/opendds)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/4647c7248ac14e7bb60c142c626ba574)](https://www.codacy.com/app/OpenDDS/OpenDDS?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=objectcomputing/OpenDDS&amp;utm_campaign=Badge_Grade)
[![Azure DevOps](https://dev.azure.com/opendds/OpenDDS/_apis/build/status/objectcomputing.OpenDDS)](https://dev.azure.com/opendds/OpenDDS/_build/latest?definitionId=1)

OpenDDS is an open-source C++ implementation of the Object Management Group's
specification "Data Distribution Service for Real-time Systems" (DDS), as well
as some other related specifications. These standards define a set of
interfaces and protocols for developing distributed applications based on the
publish-subscribe and distributed cache models. Although OpenDDS is itself
developed in C++, Java and JMS bindings are provided so that Java applications
can use OpenDDS.  OpenDDS also includes support for the DDS Security
specification.

OpenDDS is built on the [ACE](docs/dependencies.md#ace) abstraction layer to
provide platform portability.  OpenDDS also leverages capabilities of
[TAO](docs/dependencies.md#tao), such as its IDL compiler and as the basis of
the OpenDDS DCPS Information Repository (DCPSInfoRepo).

The primary development of OpenDDS was done by
[Object Computing, Incorporated](http://www.objectcomputing.com) in
St. Louis and Phoenix.  It is released under generous license
terms similar to ACE, TAO and MPC.  See the [`LICENSE`](LICENSE) file for
details.

**Table of Contents:**

* [Documentation](#documentation)
* [Support](#support)
* [Features](#features)
* [Dependencies](#dependencies)
  * [ACE/TAO](#acetao)
  * [Perl](#perl)
  * [Optional Dependencies](#optional-dependencies)
* [Supported Platforms](#supported-platforms)
  * [Operating Systems](#operating-systems)
  * [Compilers](#compilers)
* [Building and Installing](#building-and-installing)
* [Quick Start with Docker](#quick-start-with-docker)

## Documentation

- The OpenDDS Developer's Guide is freely downloadable at:
http://download.objectcomputing.com/OpenDDS/

- The TAO Developer's Guide book set may also be purchased from
https://objectcomputing.com/products/tao/tao-developers-guide

- A Doxygen for the latest release is available at
http://download.opendds.org/doxygen/.

- For developers wanting to contribute to OpenDDS, please take the time to read
[the developer's guidelines](docs/guidelines.md).

Other documentation can be found in [`docs` directory](docs).

## Support

If you encounter any problems with this release please fill out the
[PROBLEM-REPORT-FORM](PROBLEM-REPORT-FORM) file found in this directory and use
it when posting to the [mailing list](http://opendds.org/support.html) or
creating a [GitHub Issue](https://github.com/objectcomputing/OpenDDS/issues).

For commercial support please see http://opendds.org/support.html.

## Features

This release of OpenDDS is based on the DDS Specification [formal/2015-04-10
(version 1.4)](http://www.omg.org/spec/DDS/1.4).  It features the following
transport protocols:

* TCP/IP
* UDP/IP
* IP multicast
* RTPS over UDP/IP (unicast and multicast)

RTPS (Interoperability) features are based on the [DDS-RTPS Specification
formal/2019-04-03 (version 2.3)](http://www.omg.org/spec/DDSI-RTPS/2.3).  See
the OpenDDS Developer's Guide and the file [docs/design/RTPS](docs/design/RTPS)
for more details on RTPS.

See the [Developer's Guide](http://download.objectcomputing.com/OpenDDS) for
information on OpenDDS compliance with the DDS specification. If you would like
to contribute a feature or sponsor the developers to add a feature  please see
the Support section above for contact information.

## Dependencies

For a complete detailed list of dependencies, see
[`docs/dependencies.md`](docs/dependencies.md).

### ACE/TAO

OpenDDS requires TAO for both IDL compilation as well as interaction with the
DCPSInfoRepo. ACE is also required, but it is always included with TAO. If you
will be using the `configure` script for OpenDDS (see the
[`INSTALL.md`](INSTALL.md) file for details), you do not need to download TAO
first -- the `configure` script will download it for you.

There are two distributions of ACE/TAO that can be used with OpenDDS:

* OCI ACE/TAO 2.2a patch 17 or later
  * This will be automatically downloaded by default when using the configure
    script.
  * Can be manually downloaded from:
    * http://download.objectcomputing.com/TAO-2.2a_patches/
* DOC Group ACE 6.5.8 / TAO 2.5.8 or later
  * When using the configure script, DOC Group ACE/TAO can be downloaded using
    one of these arguments:
    * `--doc-group` for the latest release
    * `--ace-github-latest` to use the master branch of ACE/TAO as is. This
      also downloads the master branch of MPC as is.
  * Can be manually downloaded from:
    * https://github.com/DOCGroup/ACE_TAO/releases
    * http://download.dre.vanderbilt.edu/

### Perl

Perl is used for the configure script, running the automated tests and examples
included in this source tree, and generating Makefiles or Visual Studio project
files.

On Windows we recommend the use of [ActiveState Perl](
https://www.activestate.com/activeperl).

### Optional Dependencies

* [Google Test](docs/dependencies.md#google-test), for various tests.
  * Google Test is required for OpenDDS tests. If tests are not built, Google Test is not needed.
* [CMake](docs/dependencies.md#cmake), for building Google Test and the OpenDDS
  CMake module.
* [Java](docs/dependencies.md#java), for Java bindings.
* [Qt](docs/dependencies.md#qt), for Monitor application and ishapes demo.
* [Wireshark](docs/dependencies.md#wireshark), for the OpenDDS DCPS Wireshark
  dissector.
* [RapidJSON](docs/dependencies.md#rapidjson), for optional dissector sample
  dissection support and JSON typesupport.
* [Xerces-C++](docs/dependencies.md#xerces), for XML QoS and DDS Security.
* [OpenSSL](docs/dependencies.md#openssl), for DDS Security.

## Supported Platforms

### Operating Systems

This release of OpenDDS has been tested under the following platforms:

Linux family:
* Red Hat EL and CentOS 6.6, 6.8, 6.9 (x86\_64)
* Red Hat EL and CentOS 7.2, 7.3, 7.4 (x86\_64)
* Fedora 24 and 31 (x86\_64)
* Debian 9.4 (i686)
* Ubuntu 18.04 LTS, (x86\_64)
* openSUSE 42.1 (x86\_64)
* [Docker](docs/docker.md)

Windows family:
* Windows 7 (32-bit, 64-bit)
* Windows Server 2012 R2 (64-bit)
* Windows 10 (64-bit)

Others:
* macOS 10.15.2 (Catalina)

Embedded/Mobile/IoT:
* LynxOS-178 (OpenDDS Safety Profile)
* VxWorks 6.9 and 7 (see below)
* [Linux on Raspberry Pi](http://opendds.org/quickstart/GettingStartedPi.html)
* [Android 9.0 "Pie" (API Level 28) NDK r18b](docs/android.md)

We have built OpenDDS for VxWorks 6.9 and 7 and have run basic
system and performance tests (but not the entire regression test suite).
Please contact sales@objectcomputing.com or opendds-main@lists.sourceforge.net
for more information on support for ACE, TAO, and OpenDDS on VxWorks.
OCI's packages for ACE, TAO, and OpenDDS can be obtained on the [Wind River
Marketplace](https://marketplace.windriver.com/index.php?partners&on=details&id=33).

### Compilers

This release of OpenDDS has been tested using the following compilers:

* Microsoft Visual C++ 9 with SP1 (Visual Studio 2008)
* Microsoft Visual C++ 10 with SP1 (Visual Studio 2010)
* Microsoft Visual C++ 11 (Visual Studio 2012) - Update 4
* Microsoft Visual C++ 12 (Visual Studio 2013) - Update 5
* Microsoft Visual C++ 14 (Visual Studio 2015) - Update 3
* Microsoft Visual C++ 14.1 (Visual Studio 2017) cl 19.16.27034
* Microsoft Visual C++ 14.2 (Visual Studio 2019) cl 19.24.28316
* gcc 4.4.7, 4.8.5
* gcc 6.3
* gcc 7.2
* gcc 9.2
* Clang 6.0 (llvm.org) and 11.0 (Apple)

## Building and Installing

For building and installation instructions see the [`INSTALL.md`](INSTALL.md)
file in this directory.

## Quick Start with Docker

See [`docs/docker.md`](docs/docker.md) for how to use the pre-built docker
image.
