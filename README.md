<img src="docs/logo.svg" alt="OpenDDS" width="200">

# OpenDDS

[![Coverity Scan Build Status](https://scan.coverity.com/projects/opendds/badge.svg)](https://scan.coverity.com/projects/opendds)

OpenDDS is an open-source C++ implementation of the Object Management Group's specification "Data Distribution Service for Real-time Systems" (DDS), as well as some other related specifications.
These standards define a set of interfaces and protocols for developing distributed applications based on the publish-subscribe and distributed cache models.
Although OpenDDS is itself developed in C++, Java bindings are provided so that Java applications can use OpenDDS.
OpenDDS also includes support for the DDS Security and XTypes specifications.

OpenDDS is built on the [ACE](https://opendds.readthedocs.io/en/latest-release/devguide/building/dependencies.html#ace-tao) abstraction layer to provide platform portability.
OpenDDS also leverages the capabilities of [TAO](https://opendds.readthedocs.io/en/latest-release/devguide/building/dependencies.html#ace-tao) for IDL compilation and the OpenDDS [DCPS Information Repository](https://opendds.readthedocs.io/en/latest-release/devguide/the_dcps_information_repository.html).

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

- The OpenDDS Developer's Guide is available at https://opendds.readthedocs.io/en/latest-release.

- For developers wanting to contribute to OpenDDS, please read [the development guidelines](https://opendds.readthedocs.io/en/master/internal/dev_guidelines.html).

Other documentation can be found in [`docs` directory](docs).

## Support

For commercial support, please see https://opendds.org/support.html.

Questions concerning OpenDDS should be directed to [GitHub Discussions](https://github.com/OpenDDS/OpenDDS/discussions).

If you wish to file a bug report:

1. [Fork](https://docs.github.com/en/get-started/quickstart/fork-a-repo) OpenDDS on GitHub.
2. Add a [minimum working example](#mwe) that demonstrates the problem.
3. Create a pull request.
4. Fill out the [PROBLEM-REPORT-FORM](PROBLEM-REPORT-FORM) and attach it to the pull request.

### <a id="mwe">How to make a minimum working example</a>

1. Create a directory `tests/DCPS/MyExample` (`MyExample` is a placeholder.)
2. Add the IDL, code, and configuration files for the example programs.  Be sure to exclude any proprietary information as the submitted example will be public.
3. Add an MPC file that builds the test programs.
4. Add a launcher script `run_test.pl`.  Be sure to document the options.  See `tests/DCPS/HelloWorld/run_test.pl` for inspiration.
5. Add a `README.rst` file that gives a high-level overview of the example and the problem that it illustrates.

It should be possible to build and run the example using `run_test.pl` as in the quickstarts for [Windows](https://opendds.readthedocs.io/en/latest-release/devguide/quickstart/windows.html) and [Linux](https://opendds.readthedocs.io/en/latest-release/devguide/quickstart/linux.html).
If the problem lies in the example, then GitHub's Pull Request interface will allow people to discuss, comment on, and propose changes to get the example working.
If the problem lies in OpenDDS, then the example is a new test case.

## Features

This release of OpenDDS is based on the DDS Specification [formal/2015-04-10
(version 1.4)](https://www.omg.org/spec/DDS/1.4).  It features the following
transport protocols:

* TCP/IP
* UDP/IP
* IP multicast
* RTPS over UDP/IP (unicast and multicast)
* Shared memory

RTPS (Interoperability) features are based on the [DDS-RTPS Specification
formal/2019-04-03 (version 2.3)](https://www.omg.org/spec/DDSI-RTPS/2.3).  See
the OpenDDS Developer's Guide and the file [docs/design/RTPS](docs/design/RTPS)
for more details on RTPS.

See the [Developer's Guide](https://opendds.readthedocs.io/en/latest-release/devguide/introduction.html#specifications) for
information on OpenDDS compliance with the DDS specification. If you would like
to contribute a feature or sponsor the developers to add a feature please see
the Support section above for contact information.

## Dependencies

These are just the required dependencies. For a complete detailed list of
dependencies, including optional ones, see
https://opendds.readthedocs.io/en/latest-release/devguide/building/dependencies.html.

### ACE/TAO

OpenDDS requires TAO for both IDL compilation as well as interaction with the
DCPSInfoRepo. ACE is also required, but it is always included with TAO. If you
will be using the `configure` script for OpenDDS, you do not need to download TAO
first -- the `configure` script will download it for you.

### Perl

Perl is used for the configure script, running the automated tests and examples
included in this source tree, and generating Makefiles or Visual Studio project
files.

On Windows we recommend the use of [Strawberry Perl](https://strawberryperl.com).

## Supported Platforms

### Operating Systems

This release of OpenDDS has been tested under the following platforms:

Linux family:
* Ubuntu 22.04 LTS (x86\_64)
* Ubuntu 24.04 LTS (x86\_64)

Windows family:
* Windows Server 2022
* Windows Server 2025

MacOS family:
* MacOS 13
* MacOS 14

Embedded/Mobile/IoT:
* LynxOS-178 (OpenDDS Safety Profile)
* VxWorks 6.9, 7, 21.03 (see below)
* [Linux on Raspberry Pi](https://opendds.readthedocs.io/en/latest-release/devguide/quickstart/pi.html)
* [Android 9.0 "Pie" (API Level 28) NDK r18b](https://opendds.readthedocs.io/en/latest-release/devguide/building/android.html)

We have built OpenDDS for VxWorks 6.9, 7, and 21.03 and have run basic
system and performance tests (but not the entire regression test suite).
Please see the [OpenDDS Support page](https://opendds.org/support.html)
for more information on support for ACE, TAO, and OpenDDS on VxWorks.
Download VxWorks RPM packages for ACE, TAO, and OpenDDS [here](https://objectcomputing.com/products/opendds/vxworks).

### Compilers

This release of OpenDDS has been tested using the following compilers:

* Apple clang 15.0.0
* Microsoft Visual C++ 2022 17.13.2 (Visual Studio Enterprise 2022)
* clang 17.0.6
* clang 18.1.3
* g++ 11.4.0
* g++ 12.3.0
* g++ 13.3.0

C++17 is the minimum C++ Language Standard for building OpenDDS and applications that use OpenDDS.

## Building and Installing

For building and installation instructions see
https://opendds.readthedocs.io/en/latest-release/devguide/building/index.html

## Quick Start with Docker

See the [Docker Quick
Start](https://opendds.readthedocs.io/en/latest-release/devguide/quickstart/docker.html) for
how to use the pre-built docker image.
