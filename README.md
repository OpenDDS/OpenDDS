[![Build Status](https://travis-ci.org/objectcomputing/OpenDDS.svg?branch=master)](https://travis-ci.org/objectcomputing/OpenDDS)
[![Build status](https://ci.appveyor.com/api/projects/status/github/objectcomputing/OpenDDS?svg=true)](https://ci.appveyor.com/project/mitza-oci/opendds/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/opendds/badge.svg)](https://scan.coverity.com/projects/opendds)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/4647c7248ac14e7bb60c142c626ba574)](https://www.codacy.com/app/OpenDDS/OpenDDS?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=objectcomputing/OpenDDS&amp;utm_campaign=Badge_Grade)
[![Azure DevOps](https://dev.azure.com/opendds/OpenDDS/_apis/build/status/objectcomputing.OpenDDS)](https://dev.azure.com/opendds/OpenDDS/_build/latest?definitionId=1)

OpenDDS is an open-source C\++ implementation of the Object Management Group's
specification "Data Distribution Service for Real-time Systems".  Although
OpenDDS is itself developed in C++, Java and JMS bindings are provided so
that Java applications can use OpenDDS.  OpenDDS also includes support for the
DDS Security specification.

OpenDDS is built on the [ACE](http://www.theaceorb.com/product/aboutace.html)
abstraction layer to provide platform portability.  OpenDDS also leverages
capabilities of [TAO](http://www.theaceorb.com/), such as its IDL compiler
and as the basis of the OpenDDS DCPS Information Repository (DCPSInfoRepo).

The primary development of OpenDDS was done by
[Object Computing, Incorporated](http://www.objectcomputing.com) in
St. Louis and Phoenix.  It is released under the same generous license
terms as ACE, TAO and MPC.  See the LICENSE file for details.


# Documentation

The OpenDDS Developer's Guide is freely downloadable at:
http://download.objectcomputing.com/OpenDDS/

The TAO Developer's Guide book set may also be purchased from:
http://www.theaceorb.com/purchase/index.html


# Support

If you encounter any problems with this release please fill out the
PROBLEM-REPORT-FORM file found in this directory and use it when posting to
the [mailing list](http://opendds.org/support.html) or creating a
[GitHub Issue](https://github.com/objectcomputing/OpenDDS/issues).

For commercial support please see http://opendds.org/support.html.


# Features

This release of OpenDDS is based on the DDS Specification [formal/2015-04-10
(version 1.4)](http://www.omg.org/spec/DDS/1.4).  It features the following
transport protocols:

* TCP/IP
* UDP/IP
* IP multicast
* RTPS over UDP/IP (unicast and multicast)

RTPS (Interoperability) features are based on the [DDS-RTPS Specification
formal/2014-09-01 (version 2.2)](http://www.omg.org/spec/DDSI-RTPS/2.2).  See
the OpenDDS Developer's Guide and the file docs/design/RTPS for more details on
RTPS.

See the [Developer's Guide](http://download.objectcomputing.com/OpenDDS) for
information on OpenDDS compliance with the DDS specification. If you would like
to contribute a feature or sponsor the developers to add a feature  please see
the Support section above for contact information.


# Dependencies

## TAO

OpenDDS requires TAO for both IDL compilation as well as interaction
with the DCPSInfoRepo.  If you will be using the "configure" script for OpenDDS
(see the INSTALL file for details), you do not need to download TAO first --
the "configure" script will download it for you.

Use one of the following versions when building OpenDDS:

* TAO 2.2a patch 15
* TAO 2.5.1 (DOC Group)

Note that the 2.2a release is from OCI and can be obtained
from http://www.theaceorb.com/.  The DOC Group releases can be obtained from
http://download.dre.vanderbilt.edu/.

## GNU Make

GNU Make 3.80+ was used for automating the compiling and linking of OpenDDS
on Unix and Linux systems.

## Perl

Perl is used for running the automated tests and examples included in this
source tree and generating Makefiles or Visual Studio project files.  On Windows
we recommend the use of ActiveState Perl.  The configure script also uses Perl.


# Operating Systems

This release of OpenDDS has been tested under the following platforms:

Linux family:
* Red Hat EL and CentOS 6.6 and 6.9, x86_64
* Red Hat EL and CentOS 7.2 and 7.3, x86_64
* Fedora 24 and 28, x86_64
* Debian 9.4, i686
* Ubuntu 16.04 LTS, x86_64
* openSUSE 42.1, and 42.2, x86_64

Windows family:
* Windows 7 (32-bit, 64-bit)
* Windows Server 2012 R2 (64-bit)
* Windows 10 (64-bit)

Others:
* macOS 10.13 (High Sierra)

Embedded/Mobile/IoT:
* LynxOS-178 (OpenDDS Safety Profile)
* VxWorks 6.9 and 7 (see below)
* Linux on Raspberry Pi
* Android NDK r12b (ARM)

We have built OpenDDS for VxWorks 6.9 and 7 and have run basic
system and performance tests (but not the entire regression test suite).
Please contact sales@objectcomputing.com or opendds-main@lists.sourceforge.net
for more information on support for ACE, TAO, and OpenDDS on VxWorks.
OCI's packages for ACE, TAO, and OpenDDS can be obtained on the Wind River
Marketplace at:
https://marketplace.windriver.com/index.php?partners&on=details&id=33


# Compilers

This release of OpenDDS has been tested using the following compilers:

* Microsoft Visual C++ 9 with SP1 (Visual Studio 2008)
* Microsoft Visual C++ 10 with SP1 (Visual Studio 2010)
* Microsoft Visual C++ 11 (Visual Studio 2012) - Update 4
* Microsoft Visual C++ 12 (Visual Studio 2013) - Update 5
* Microsoft Visual C++ 14 (Visual Studio 2015) - Update 3
* Microsoft Visual C++ 14.1 (Visual Studio 2017) cl 19.12.25835
* gcc 4.4.7, 4.8, 4.9
* gcc 5.4
* gcc 6.2, 6.3
* gcc 7.2
* gcc 8.1, 8.2
* Clang 6.0 (llvm.org) and 9.0 (Apple)


# Building and Installing

For building and installation instructions
see the INSTALL file in this directory.

# Quick Start with Docker

Docker images containing a pre-built OpenDDS are available on [DockerHub](https://hub.docker.com/r/objectcomputing/opendds/).  An image corresponding to a particular release has a tag of the form `release-DDS-X.xx`, e.g., `release-DDS-3.12`.

1. Check for prerequisites

        docker --version
        docker-compose --version

2. Enter a container

        docker run --rm -ti -v "$PWD:/opt/workspace" objectcomputing/opendds

3. Copy the `Messenger` directory which contains an example from the [Developer's Guide](http://download.objectcomputing.com/OpenDDS/OpenDDS-latest.pdf)

        cp -R /opt/OpenDDS/DevGuideExamples/DCPS/Messenger Messenger
        cd Messenger

4. Configure and build the Messenger example

        mwc.pl -type gnuace
        make

5. Exit the container

        exit

6. Enter the `Messenger` directory

        cd Messenger

7. Create an `rtps.ini` file to control discovery with the following content

        [common]
        DCPSGlobalTransportConfig=$file
        DCPSDefaultDiscovery=DEFAULT_RTPS

        [transport/the_rtps_transport]
        transport_type=rtps_udp

8. Run the Messenger example with RTPS

        docker-compose up

9. Run the Messenger example with InfoRepo

        docker-compose -f docker-compose-inforepo.yml up
        # Use Control-C to kill the InfoRepo process

# OpenDDS Compliance with the DDS Specification

See http://www.opendds.org and the OpenDDS Developer's Guide at:
http://download.objectcomputing.com/OpenDDS/OpenDDS-latest.pdf
