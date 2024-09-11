.. _deps:

############
Dependencies
############

********************************************
Required to Build the Core OpenDDS Libraries
********************************************

.. note:: Perl is required to run the configure script; MPC, ACE, and TAO will be downloaded automatically by the configure script by default.

.. _deps-perl:

Perl
====

`Perl <https://www.perl.org/>`__ is an interpreted language used in the configure script, the tests, and most other scripting in OpenDDS codebase.
Even if the configure script is not used, it is also required to run MPC, so it is required to build OpenDDS.

Perl should be 5.18 or newer and be available on the system ``PATH``.
Older versions of Perl will probably work, but are not tested anymore.
`Strawberry Perl <https://strawberryperl.com>`__ is recommended for Windows.

Testing scripts are written in Perl and use the common PerlACE modules provided by ACE.
For scripts that will be part of automated testing, don't assume the presence of any non-standard (CPAN) modules.
Perl offers many facilities for portability.
Shell scripts are by definition non-portable and should not to be committed to the OpenDDS repository.

Perl core modules are a required part of Perl.
Some Linux distributions install the Perl interpreter without including core modules.
Using OpenDDS with this sort of partial Perl installation may fail unexpectedly when using configure, MPC, ``make depend``, or test scripts.

.. _deps-mpc:

MPC
===

MPC is the build system used by OpenDDS, used to configure the build and generate platform specific build files (Makefiles, VS solution files, etc.).

The official repository is hosted on Github at `DOCGroup/MPC <https://github.com/DOCGroup/MPC>`__.

It is included in the release archive of ACE/TAO, which is downloaded by the configure script by default.

.. _deps-ace-tao:

ACE/TAO
=======

The DOC Group repository for ACE/TAO is hosted on Github at `DOCGroup/ACE_TAO <https://github.com/DOCGroup/ACE_TAO>`__.

These are the versions of ACE/TAO that are officially supported by OpenDDS in this release (|release|):

.. _ace6tao2:

DOC Group :acetaorel:`ace6tao2`
  The configure script will download this version by default.
  :ref:`CMake <cmake-building>` will download this version if :cmake:var:`OPENDDS_ACE_TAO_KIND` is set to ``ace6tao``.

  Pass ``--ace-github-latest`` to the configure script to clone the ``ace6tao2`` branch of ACE/TAO as is.
  This also clones the ``master`` branch of MPC as is.
  :ref:`CMake <cmake-building>` will do the same if :cmake:var:`OPENDDS_ACE_TAO_KIND` is set to ``ace6tao`` and :cmake:var:`OPENDDS_ACE_TAO_GIT` is set to ``TRUE``.

.. _ace7tao3:

DOC Group :acetaorel:`ace7tao3`
  Pass ``--doc-group3`` to the configure script to download this version.
  :ref:`CMake <cmake-building>` will download this version by default.

  This version requires a C++14-capable compiler.

  Pass ``--ace-github-latest`` to the configure script to clone the ``master`` branch of ACE/TAO as is.
  This also clones the ``master`` branch of MPC as is.
  :ref:`CMake <cmake-building>` will do the same if :cmake:var:`OPENDDS_ACE_TAO_GIT` is set to ``TRUE``.

.. _ace8tao4:

DOC Group :acetaorel:`ace8tao4`
  Pass ``--ace-tao=ace8tao4`` to the configure script to download this version.
  :ref:`CMake <cmake-building>` will download this version if :cmake:var:`OPENDDS_ACE_TAO_KIND` is set to ``ace8tao4``.

  This version requires a C++17-capable compiler.

.. _deps-ace:

ACE
---

ACE is the platform abstraction layer used by OpenDDS.
It is used both directly and through TAO. Facilities not provided by the C++ 2003 standard library, for example sockets, threads, and dynamic library loading, are provided by ACE.

Some other features OpenDDS relies on ACE for:

- ACE provides the ``gnuace`` type used by MPC for generating Makefiles for OpenDDS
- ACE contains a script, ``generate_export_file.pl``, which is used (along with MPC) to manage shared libraries' symbol visibility (also known as export/import)

  - See ACE documentation and usage guidelines for details

- ACE logging is used (``ACE_Log_Msg`` and related classes).

  - This is used through the ``ACE_DEBUG`` and ``ACE_ERROR`` macros.
  - ACE logging uses a formatting string that works like ``std::printf()`` but not all of the formatting specifiers are the same as ``printf()``.
    Please read the ``ACE_Log_Msg`` documentation before using.
  - The most commonly misused formatting specifier is ``%s``.
    In ``printf`` this is for ``char*`` C strings, but in ``ACE_Log_Msg`` this is for ``ACE_TCHAR*`` C strings.
    ``ACE_TCHAR`` can be ``char`` or a wide character depending on how ACE was built (see next point).
    ``%C`` should be used for strings that are always ``char*``, like ``std::string::c_str()``.

- ACE has classes and macros for wide/narrow string conversion.
  See :ghfile:`docs/design/WCHAR` for details.
- ACE provides support for platforms that have a non-standard program entry point (``main``).
  All of our ``main`` functions are ``int ACE_TMAIN(int argc, ACE_TCHAR* argv[])``.

.. _deps-tao:

TAO
---

TAO is a C++ CORBA Implementation built on ACE.

- TAO provides the ``tao_idl`` IDL compiler and non-generated classes which implement the IDL-to-C++ mapping.
- TAO ORBs are only created for interaction with the DCPSInfoRepo, all other uses of TAO are basic types and local interfaces.
- A separate library, ``OpenDDS_InfoRepoDiscovery``, encapsulates the participant process's use of the ORB.

  - This is the only library which depends on ``TAO_PortableServer``.

The TAO Developer's Guide book can be requested for free from https://objectcomputing.com/platforms/tao/tao-developers-guide.
The CORBA Programmers Guide can be downloaded for free from https://www.remedy.nl/opensource/corbapg.html.

.. _deps-optional:

*********************
Optional Dependencies
*********************

.. _deps-cmake:

CMake
=====

OpenDDS has a :ghfile:`package included <cmake>` for `CMake <https://cmake.org/>`__.
See :doc:`cmake` for how to build OpenDDS applications with CMake and without the need to use MPC in your application.

CMake is required to build :ref:`deps-gtest` for OpenDDS tests if a prebuilt GoogleTest is not found or provided.
See :ghfile:`tests/gtest_setup.txt` for details.

CMake should be version 3.3 or later for :doc:`cmake`.
It should be version 3.23 or later for :ref:`cmake-building`.

.. _deps-gtest:

GoogleTest
==========

`GoogleTest <https://google.github.io/googletest/>`__ is required for OpenDDS tests.

GoogleTest is a git submodule that will be downloaded automatically if the repository was recursively cloned or submodules were initialized separately.

.. note:: If OpenDDS is not a git repository or Git isn't available, GoogleTest will have to be downloaded separately and configured manually.

See :ghfile:`tests/gtest_setup.txt` for details.

.. _deps-java:

Java
====

OpenDDS has optional :ref:`Java bindings <java>`.
It requires the Java Development Kit (JDK).

There is also support for Java Message Server (JMS) v1.1.
In addition to the JDK, it requires Ant and JBoss 4.2.x.
See :ghfile:`java/jms/README`.

.. _deps-qt:

Qt
==

`Qt5 <https://www.qt.io/>`__ is used for the :ghfile:`tools/monitor` utility program and the :ghfile:`examples/DCPS/ishapes` RTPS demo.

See :ghfile:`docs/qt.md` for details on configuring OpenDDS to use Qt.

.. _deps-wireshark:

Wireshark
=========

A `Wireshark <https://www.wireshark.org/>`__ dissector plugin for OpenDDS' non-RTPS transports is included with OpenDDS.
The dissector supports Wireshark 1.2 and onwards and supports displaying and filtering by sample contents and from Wireshark 1.12 onwards.

Because of Wireshark's use of Glib, Glib is also required to build the dissector.

See :ghfile:`tools/dissector/README.md` for details.

.. _deps-rapidjson:

RapidJSON
=========

`RapidJSON <https://rapidjson.org/>`__ is a C++ JSON Library used for :ghfile:`sample dissection in the Wireshark dissector <tools/dissector/README.md#sample-dissection>` and RapidJSON type support.
Support for RapidJSON, if available, is enabled by default unless ``--no-rapidjson`` was passed.

RapidJSON is a git submodule that will be downloaded automatically if the repository was recursively cloned or submodules were initialized separately.

.. note:: If OpenDDS is not a git repository or Git isn't available, RapidJSON will have to be downloaded separately and configured manually.

.. _deps-xerces:

Xerces
======

`Apache Xerces <https://xerces.apache.org/xerces-c/>`__ ("Xerces 3 C++" specifically) is used for parsing QoS XML and :ref:`sec` XML configuration files.

.. _deps-openssl:

OpenSSL
=======

`OpenSSL <https://www.openssl-library.org/>`__ is used for :ref:`sec` for verifying security configurations and encryption and decryption.
Versions 1.0, 1.1 and 3.0 (3.0.1 or later) are supported.

.. _deps-python:

Python
======

`Python <https://www.python.org/>`__ is used for some scripts where Perl isn't as suitable.
Most notably this includes :doc:`this Sphinx-based documentation </internal/docs>` and processing the results of the CMake tests in :ghfile:`tests/auto_run_tests.pl` if ``--cmake`` is passed.

Unless noted otherwise, Python should be version 3.10 or later.

Because it's an optional dependency, Python should not be required for any script used for building and testing the core functionality of OpenDDS.
Right now only Perl can be used for situations like that.
