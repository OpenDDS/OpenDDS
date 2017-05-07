[![Build Status](https://travis-ci.org/objectcomputing/OpenDDS.svg?branch=master)](https://travis-ci.org/objectcomputing/OpenDDS)
[![Build status](https://ci.appveyor.com/api/projects/status/github/objectcomputing/OpenDDS?svg=true)](https://ci.appveyor.com/project/mitza-oci/opendds/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/opendds/badge.svg)](https://scan.coverity.com/projects/opendds)

OpenDDS is an open-source C\++ implementation of the Object Management Group's
specification "Data Distribution Service for Real-time Systems".  OpenDDS
provides C\++, Java and JMS binding.

OpenDDS is built on the [ACE](http://www.dre.vanderbilt.edu/~schmidt/ACE.html)
abstraction layer to provide platform portability.  OpenDDS also leverages
capabilities of [TAO](http://www.dre.vanderbilt.edu/~schmidt/TAO.html), such as its IDL compiler
and as the basis of the OpenDDS DCPS Information Repository (DCPSInfoRepo).

The initial development of OpenDDS was done by the ACE/TAO development
team at [Object Computing, Incorporated](http://www.objectcomputing.com) in
St. Louis and Phoenix.  It is released under the same generous license
terms as ACE, TAO and MPC.  See the LICENSE file for details.


# Documentation

For the development of Java applications see java/README and java/jms/README for details.

The [OpenDDS Developer's Guide](http://download.objectcomputing.com/OpenDDS/OpenDDS-latest.pdf) is freely downloadable.

The [TAO Developer's Guide](http://www.theaceorb.com/purchase/index.html) may also be purchased as
additional documentation.


# Support

The OpenDDS community strives to make OpenDDS as bug free as possible.  If you encounter
any problems with this release please fill out the PROBLEM-REPORT-FORM
file found in this directory and report an issue using
[github](https://github.com/objectcomputing/OpenDDS/issues).

If you desire responsive commercial support for OpenDDS on any problems
you encounter, we strongly recommend you to use the support options as described on
the [OpenDDS support page](http://opendds.org/support.html).


# Features

This release of OpenDDS is based on the DDS Specification
[formal/2015-04-10 (version 1.4)](http://www.omg.org/spec/DDS/1.4).
It features the following transport protocols:

* TCP/IP
* UDP/IP
* IP multicast
* RTPS over UDP/IP (unicast and multicast)

RTPS (Interoperability) features are based on the
[DDS-RTPS Specification formal/2014-09-01 (version 2.2)](http://www.omg.org/spec/DDSI-RTPS/2.2/).
See the OpenDDS Developer's Guide and
the file docs/design/RTPS for more details on RTPS.

Currently OpenDDS does not support the Data Local Reconstruction Layer
(DLRL) portion of the DDS specification.  See the Developer's Guide for
further information on OpenDDS compliance with the DDS specification.
This document is freely available as a PDF file
from the same location where OpenDDS can be downloaded.  Some
additional notes about compliance with the DDS specification can be
found below.  If you would like to have add a feature to OpenDDS
please see the Support section for the available support options.


# Dependencies

## TAO

OpenDDS requires TAO for both IDL compilation as well as interaction
with the DCPSInfoRepo.  If you will be using the "configure" script for OpenDDS
(see the INSTALL file for details), you do not need to download TAO first --
the "configure" script will download it for you.

At a minimum, you must be at one of the following versions in order to properly
compile OpenDDS:

* TAO 2.2a patch 11
* TAO 2.4.2 (DOC Group)

Note that the 2.2a is from OCI and can be obtained
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
* Red Hat EL 5 and 5.3, x86_64
* Red Hat EL and CentOS 6.6 and 6.8, x86_64
* Red Hat EL and CentOS 7.2 and 7.3, x86_64
* Fedora Core 6, x86
* Fedora 24, and 25, x86_64
* Ubuntu 16.04 LTS, x86_64
* openSUSE 42.1, and 42.2 , x86_64

Windows family:
* Windows 7 (32-bit, 64-bit)
* Windows Server 2012 R2 (64-bit)
* Windows 10 (64-bit)

Others:
* SunOS 5.10 (Solaris 10) (SPARC)
* Mac OSX 10.11 (El Capitan)

Embedded/Mobile/IoT:
* LynxOS-178 (OpenDDS Safety Profile)
* VxWorks 6.9 and 7 (see below)
* Linux on Raspberry Pi and Intel Edison
* Android NDK r12b (ARM)

We have built OpenDDS for VxWorks 6.9 and 7 and have run basic
system and performance tests (but not the entire regression test suite).
Please contact sales@objectcomputing.com or opendds-main@lists.sourceforge.net for
more information on support for ACE, TAO, and OpenDDS on VxWorks.
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
* Microsoft Visual C++ 15 (Visual Studio 2017)
* gcc 4.1.x
* gcc 4.4.x
* gcc 4.8.x
* gcc 4.9.x
* gcc 5.4
* gcc 6.2
* gcc 6.3
* Clang 3.9 (llvm.org) and 7.3 (Apple)
* Sun C++ 5.9 SunOS_sparc Patch 124863-01 2007/07/25

# Building and Installing

For building and installation instructions
see the INSTALL file in this directory.


# OpenDDS Compliance with the DDS Specification

See http://www.opendds.org and the OpenDDS Developer's Guide at:
http://download.objectcomputing.com/OpenDDS/OpenDDS-latest.pdf
