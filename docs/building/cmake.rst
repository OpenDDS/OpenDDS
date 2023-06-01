################################
Using OpenDDS in a CMake Project
################################

OpenDDS can be used with `CMake <https://cmake.org>`__\-based projects by using the :ghfile:`OpenDDS CMake config package <cmake>`.
This package bridges the gap between the MPC build system used by OpenDDS and CMake-based projects by providing imported library targets and the ability to add IDL to a target using :cmake:func:`opendds_target_sources`.

************
Requirements
************

CMake version 3.3.2 or greater is required to use the CMake package, but some some features require newer versions.

*******************************
Using the OpenDDS CMake Package
*******************************

Examples
========

Developer’s Guide Messenger Example
-----------------------------------

For a simple quick-start example of a ``CMakeLists.txt`` using OpenDDS with CMake see the :ghfile:`Developer’s Guide Messenger example <DevGuideExamples/DCPS/Messenger/CMakeLists.txt>`.
The following instructions show how to configure and build it:

.. tab:: Linux, macOS, BSDs, etc.

  Make sure the environment is setup by using ``source setenv.sh``.

  .. code:: bash

     cd DevGuideExamples/DCPS/Messenger
     mkdir build
     cd build
     cmake ..
     cmake --build .
     perl run_test.pl

.. tab:: Windows

  Make sure the environment is setup by using ``call setenv.cmd``.
  The following assumes Visual Studio 2017 using 64-bit architecture (adjust the CMake ``-G`` parameter if using something different).

  .. code:: bat

     cd DevGuideExamples\DCPS\Messenger
     mkdir build
     cd build
     cmake -G "Visual Studio 15 2017 Win64" ..
     cmake --build .
     perl run_test.pl

Example Using Installed OpenDDS (Unix only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``--prefix`` switch can be passed to configure to enable the ``install`` target, which will install OpenDDS (including the OpenDDS CMake config package) into the specified directory.
See :ref:`install` for details.

.. note:: Be sure to pass an absolute path to ``--prefix``.

.. code:: bash

   OPENDDS_PREFIX="$PWD/opendds-install"
   DDS_ROOT="$PWD/OpenDDS"
   ACE_ROOT="$DDS_ROOT/ACE_wrappers"
   cd OpenDDS
   ./configure --prefix="$OPENDDS_PREFIX"
   make -j $(getconf _NPROCESSORS_ONLN)
   make install
   cd DevGuideExamples/DCPS/Messenger
   mkdir build
   cd build
   cmake -DCMAKE_PREFIX_PATH="$OPENDDS_PREFIX" ..
   cmake --build .
   PERL5LIB="$DDS_ROOT/bin:$ACE_ROOT/bin" LD_LIBRARY_PATH="$OPENDDS_PREFIX/lib:$LD_LIBRARY_PATH" perl run_test.pl

Other Examples
--------------

The :ghfile:`CMake tests <tests/cmake>` are written primarily as tests, but can also be used as examples for specific features and approaches.

find_package
============

To use the OpenDDS CMake package, first it has to be loaded using `find_package <https://cmake.org/cmake/help/latest/command/find_package.html>`__.
For example to mandatorily load OpenDDS:

.. code:: cmake

   find_package(OpenDDS REQUIRED)

For this to work, CMake has to be able to find the package.
If the OpenDDS build environment variables (from ``source setenv.sh`` or ``call setenv.cmd``) are set then CMake should be able to find it using the ``PATH`` environment variable.
If those environment variables aren't set, OpenDDS was installed to a path CMake doesn't search automatically, or CMake can’t otherwise find the OpenDDS package, then CMake has to be told about it explicitly somehow.
This can be done a number of ways, which includes adding the OpenDDS source tree or install prefix path to ``CMAKE_PREFIX_PATH`` or setting ``OPENDDS_ROOT`` to that path (if using CMake 3.12 or later).

Consult the ``find_package`` documentation for your CMake version for all the details on how CMake could find OpenDDS.
`Here <https://cmake.org/cmake/help/latest/command/find_package.html#config-mode-search-procedure>`__ is the documentation for the latest version of CMake

.. _cmake-components:

Components
----------

.. versionadded:: 3.25

Arguments can be passed to ``find_package(OpenDDS COMPONENTS <argument>...)`` (or alternatively ``find_package(OpenDDS REQUIRED [COMPONENTS] <argument>...)``) to change what is loaded and considered required.
These can be:

- :ref:`Libraries <cmake-libraries>`

  - The ``OpenDDS::`` prefix is optional, but ``ACE::`` and ``TAO::`` libraries are not.
  - Ex:

    .. code-block:: cmake

      find_package(OpenDDS REQUIRED Security OpenDDS::Shmem ACE::XML_Utils)

- :ref:`Features <cmake-feature-vars>`

  - Feature names should be the same as the variable, but without the leading ``OPENDDS_`` and all lowercase.
  - Feature names by themselves will be required to be enabled.
  - If the name is followed by ``=``, then the `CMake boolean value <https://cmake.org/cmake/help/latest/command/if.html#basic-expressions>`__ following that determines if the feature must be enabled or disabled.
  - Ex:

    .. code-block:: cmake

      find_package(OpenDDS REQUIRED built_in_topics safety_profile=OFF)

- ``NO_OPENDDS`` skips loading OpenDDS libraries, which allows using ACE/TAO without OpenDDS being built.
  ``NO_TAO`` also skips loading TAO libraries, leaving just ACE libraries.

  - ``NO_OPENDDS`` will imply :cmake:func:`opendds_target_sources(SKIP_OPENDDS_IDL)`.

Currently all arguments passed to ``OPTIONAL_COMPONENTS`` are ignored as all libraries are treated as optional.

Adding IDL Sources with opendds_target_sources
==============================================

Aside from importing the various OpenDDS targets, the OpenDDS config package provides an easy way to add IDL sources to CMake targets.
This is achieved by :cmake:func:`opendds_target_sources`.
For example, from the Developer’s Guide Messenger example:

.. literalinclude:: ../../DevGuideExamples/DCPS/Messenger/CMakeLists.txt
  :language: cmake
  :start-at: IDL TypeSupport Library
  :end-before: set(opendds_libs

Here the IDL is added to a library that is shared by the executables, but ``opendds_target_sources`` can also be used on executables directly.

.. note:: CMake version 3.10 and below will issue a harmless warning if ``add_library`` is called without any sources.

.. _cmake-install-import-runtime-artifacts:

install(IMPORTED_RUNTIME_ARTIFACTS)
===================================

.. versionadded:: 3.20

If using CMake 3.21 or later, it’s possible to install shared libraries from OpenDDS, ACE, and TAO in CMake along side the application using `install(IMPORTED_RUNTIME_ARTIFACTS) <https://cmake.org/cmake/help/latest/command/install.html#install-imported-runtime-artifacts>`__.
This will just install shared libraries, not any development dependencies like ``opendds_idl`` or static libraries.

If OpenDDS and ACE/TAO is built with ``clang``, the shared libraries might be missing an ``SONAME`` entry.
Is is `an issue with ACE/TAO <https://github.com/DOCGroup/ACE_TAO/issues/1790>`__.
If trying to use ``install(IMPORTED_RUNTIME_ARTIFACTS)`` in this case, it causes the dynamic linker to ignore the libraries and report that they could not be found.
One workaround is to add ``SOFLAGS+=-Wl,-h,$(SONAME)`` to ``$ACE_ROOT/include/makeinclude/platform_macros.GNU`` before building.
This can be done manually after running the configure script or by passing ``--macros=SOFLAGS+=-Wl,-h,\$\(SONAME\)`` to the configure script.

:cmake:func:`opendds_get_library_dependencies` is provided to help find out what libraries need to be installed.

See the :ghfile:`install Test <tests/cmake/install/library/CMakeLists.txt>` for an example of using this.

.. _cmake-installing-generated-interface-files:

Installing Generated Interface Files
====================================

.. versionadded:: 3.20

It is possible to install files from the ``OPENDDS_*_INTERFACE_FILES`` target properties for downstream projects to use.
See the :ghfile:`install Test <tests/cmake/install/library/CMakeLists.txt>` for an example of this.
It uses `install(FILES) <https://cmake.org/cmake/help/latest/command/install.html#files>`__, but there isn't any restriction on what installation method can be used.
For example, the `PUBLIC_HEADER <https://cmake.org/cmake/help/latest/prop_tgt/PUBLIC_HEADER.html>`__ target property could be set on target to the desired files from the interface lists.
Then they installed using `install(TARGETS ... PUBLIC_HEADER ...) <https://cmake.org/cmake/help/latest/command/install.html#installing-targets>`__.

Manually Creating config.cmake
==============================

The :ghfile:`configure script <configure>` is responsible for generating the ``config.cmake`` file in :ghfile:`cmake`, which has various configuration options.
These options provide the OpenDDS CMake package with the required context it needs to integrate with the OpenDDS code generators and libraries.

If you are using OpenDDS libraries that were built without the help of the ``configure`` script, the ``config.cmake`` file needs to be created manually.
See :ref:`cmake-config-vars` for all the possible values to set.

*********
Reference
*********

.. _cmake-libraries:

Libraries
=========

The CMake package provides the following library targets for OpenDDS that can be linked using `target_link_libraries <https://cmake.org/cmake/help/latest/command/target_link_libraries.html>`__:

- ``OpenDDS::Dcps``

  - Core OpenDDS Library

- ``OpenDDS::Rtps``

  - RTPS Discovery

- ``OpenDDS::InfoRepoDiscovery``

  - InfoRepo Discovery

- ``OpenDDS::Rtps_Udp``

  - RTPS Transport

- ``OpenDDS::Multicast``

  - Multicast Transport

- ``OpenDDS::Shmem``

  - Shared Memory Transport

- ``OpenDDS::Tcp``

  - TCP Transport

- ``OpenDDS::Udp``

  - UDP Transport

- ``OpenDDS::Security``

  - :doc:`/devguide/dds_security`

It also provides libraries from ACE/TAO:

- ``ACE::ACE``
- ``ACE::XML_Utils``
- ``TAO::TAO``
- ``TAO::IDL_FE``
- ``TAO::AnyTypeCode``
- ``TAO::BiDirGIOP``
- ``TAO::CodecFactory``
- ``TAO::IORManip``
- ``TAO::IORTable``
- ``TAO::ImR_Client``
- ``TAO::PI``
- ``TAO::PortableServer``
- ``TAO::Svc_Utils``
- ``TAO::Valuetype``

Functions
=========

.. cmake:func:: opendds_target_sources

  ::

    opendds_target_sources(<target>
      [<idl-file>...]
      [PRIVATE|PUBLIC|INTERFACE <idl-file>...]
      [TAO_IDL_OPTIONS <option>...]
      [OPENDDS_IDL_OPTIONS <option>...]
      [SUPPRESS_ANYS TRUE|FALSE]
      [ALWAYS_GENERATE_LIB_EXPORT_HEADER TRUE|FALSE]
      [GENERATE_SERVER_SKELETONS TRUE|FALSE]
      [AUTO_LINK TRUE|FALSE]
      [SKIP_TAO_IDL]
      [SKIP_OPENDDS_IDL]
    )

  A function that acts like `target_sources <https://cmake.org/cmake/help/latest/command/target_sources.html>`__, but it adds IDL files and the resulting generated code to a target.

  ``<idl file>...`` are IDL files that can be absolute or relative to `CMAKE_CURRENT_SOURCE_DIR <https://cmake.org/cmake/help/latest/variable/CMAKE_CURRENT_SOURCE_DIR.html>`__.
  Each one will generate code using ``tao_idl`` and ``opendds_idl`` that is added to the target.
  The optional scope-qualifier (``PRIVATE``, ``PUBLIC``, ``INTERFACE``) sets the scope of the generated files.
  When it is omitted, :cmake:var:`OPENDDS_DEFAULT_SCOPE` is used.

  .. deprecated:: 3.15

    C/C++ files can also be passed along with IDL files, but this has been deprecated.

  .. cmake:func:arg:: TAO_IDL_OPTIONS <option>...

    Pass options to :term:`tao_idl`.
    Valid options can be found `here <https://htmlpreview.github.io/?https://github.com/DOCGroup/ACE_TAO/blob/Latest_Micro/TAO/docs/compiler.html>`__

  .. cmake:func:arg:: OPENDDS_IDL_OPTIONS <option>...

    Pass options to :doc:`/devguide/opendds_idl`.
    Add ``OPENDDS_IDL_OPTIONS -Lc++11`` to use the :ref:`C++11 IDL Mapping <opendds_idl--using-the-idl-to-c-11-mapping>`.

  .. cmake:func:arg:: SUPPRESS_ANYS TRUE|FALSE

    If ``FALSE``, TAO TypeCode for ``any`` will be generated.
    The default is set by :cmake:var:`OPENDDS_SUPPRESS_ANYS`.

    .. versionadded:: 3.17

  .. cmake:func:arg:: ALWAYS_GENERATE_LIB_EXPORT_HEADER TRUE|FALSE

    If ``TRUE``, an header for exporting symbols in a shared library will be always generated as long the target is `some sort of library <https://cmake.org/cmake/help/latest/prop_tgt/TYPE.html>`__.
    This is only really useful if the target is a library that uses the export header itself and also needs to be built as a static library as well.
    If ``FALSE``, then it will only be done if the target is a shared library.
    The default is set by :cmake:var:`OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER`.

    .. versionadded:: 3.20

  .. cmake:func:arg:: GENERATE_SERVER_SKELETONS TRUE|FALSE

    ``tao_idl`` generate code for CORBA servers.
    The default is ``FALSE``.
    ``tao_idl`` by itself does this by default, but by default ``opendds_target_sources`` passes ``-SS`` to suppress this as it's not normally useful to an OpenDDS application.

    .. versionadded:: 3.25

  .. cmake:func:arg:: AUTO_LINK TRUE|FALSE

    Automatically link ``OpenDDS::Dcps`` or other dependencies to the target using the "max" scope.
    The default is set by :cmake:var:`OPENDDS_AUTO_LINK_DCPS`.

    .. versionadded:: 3.25

  .. cmake:func:arg:: SKIP_TAO_IDL

    Skip invoking ``tao_idl`` on the IDL files.

    .. versionadded:: 3.25

  .. cmake:func:arg:: SKIP_OPENDDS_IDL

    Skip invoking ``opendds_idl`` on the IDL files.

    .. versionadded:: 3.25

  After ``opendds_target_sources`` is run on a target, it will have these target properties set on it:

  .. cmake:prop:: OPENDDS_LANGUAGE_MAPPINGS

    This holds the IDL language mappings used in the target based on what is passed to :cmake:func:`opendds_target_sources(OPENDDS_IDL_OPTIONS)`.

    It will be a list that can contain one or more of the following:

    - ``"C++03"``

      - IDL-to-C++ mapping generated by default.

    - ``"C++11"``

      - IDL-to-C++11 mapping available when passing ``-Lc++11``.

    - ``"FACE"``

      - Will appear if ``-Lface`` is passed.

    - ``"Java"``

      - Currently unsupported.

    If the CMake version is at least 3.12, then this property will be exported with the target.

    .. versionadded:: 3.15

  .. cmake:prop:: OPENDDS_GENERATED_DIRECTORY

    The is the directory where generated files have been placed.
    This is an absolute path and is not exported with the target.
    See :ref:`cmake-installing-generated-interface-files` for more information.

    .. versionadded:: 3.20

  The following ``OPENDDS_*_INTERFACE_FILES`` target properties are used to help with :ref:`cmake-installing-generated-interface-files`.
  Some notes about these properties:

  - All paths are absolute.
  - All the generated files will be somewhere within the path from the :cmake:prop:`OPENDDS_GENERATED_DIRECTORY` target property of the target.
  - All the properties have the ``INTERFACE`` in their name, but this includes ``PUBLIC`` scoped files as ``PUBLIC`` implies ``INTERFACE`` in CMake.
    ``PRIVATE`` scoped files are excluded from these lists as they shouldn't have a use outside the target.
  - These properties are not exported with the target because the paths may not be valid any more if the build directory has been removed or the export is being used on another machine.

  .. cmake:prop:: OPENDDS_PASSED_IDL_INTERFACE_FILES

    The ``PUBLIC`` and ``INTERFACE`` scoped IDL files passed.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_GENERATED_IDL_INTERFACE_FILES

    The IDL files generated from the IDL files in :cmake:prop:`OPENDDS_PASSED_IDL_INTERFACE_FILES`.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_ALL_IDL_INTERFACE_FILES

    Combination of :cmake:prop:`OPENDDS_PASSED_IDL_INTERFACE_FILES` and :cmake:prop:`OPENDDS_GENERATED_IDL_INTERFACE_FILES`.

  .. cmake:prop:: OPENDDS_GENERATED_HEADER_FILES

    The ``.h`` and ``.inl`` files generated from :cmake:prop:`OPENDDS_ALL_IDL_INTERFACE_FILES`.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_ALL_GENERATED_INTERFACE_FILES

    Combination of :cmake:prop:`OPENDDS_GENERATED_IDL_INTERFACE_FILES` and :cmake:prop:`OPENDDS_GENERATED_HEADER_FILES`.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_ALL_INTERFACE_FILES

    All the ``INTERFACE`` and ``PUBLIC`` scoped files that were passed in or generated.

    .. versionadded:: 3.20

  .. versionadded:: 3.14

  .. versionchanged:: 3.25

    ``OPENDDS_TARGET_SOURCES`` is now called ``opendds_target_sources``, but this shouldn't affect anything because functions and macros are case-insensitive in CMake.

.. cmake:func:: opendds_get_library_dependencies

  ::

    opendds_get_library_dependencies(<output-list-var-name> <lib>...)

  A function to help when using :ref:`cmake-install-import-runtime-artifacts`.
  The variable to create in the caller's scope that will contain the list of all targets passed and their ACE, TAO, and OpenDDS dependencies.

  .. versionadded:: 3.20

Variables
=========

Package Options
---------------

These variables can be used to override default behavior of the CMake package.

.. cmake:var:: OPENDDS_CMAKE_VERBOSE

  If ``TRUE``, then log detailed status information at configure-time.
  The default for this is ``FALSE``.

.. cmake:var:: OPENDDS_DEFAULT_NESTED

  If ``TRUE``, then :term:`topic types <topic type>` must be declared explicitly using :ref:`annotations <xtypes--indicating-which-types-can-be-topic-types>`.
  The default for this is ``TRUE``.

  This can also be controlled on a finer level by passing ``--default-nested`` or ``--no-default-nested`` to :cmake:func:`opendds_target_sources(OPENDDS_IDL_OPTIONS)`.
  For example:

  .. code:: cmake

     add_library(messenger)
     opendds_target_sources(messenger
       PUBLIC
         Messenger.idl
         OPENDDS_IDL_OPTIONS --no-default-nested
     )

.. cmake:var:: OPENDDS_FILENAME_ONLY_INCLUDES

  Setting this to ``TRUE`` tells OpenDDS’s IDL compiler to strip path information from ``#include`` lines in generated files.
  Turning the option on can make it easier to specify build rules for IDL files that include other IDL files.
  The default for this is ``FALSE``.

  .. versionadded:: 3.15

.. cmake:var:: OPENDDS_SUPPRESS_ANYS

  Default value for :cmake:func:`opendds_target_sources(SUPPRESS_ANYS)`.
  The default for this is ``TRUE``.

  .. versionadded:: 3.17

.. cmake:var:: OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER

  Default value for :cmake:func:`opendds_target_sources(ALWAYS_GENERATE_LIB_EXPORT_HEADER)`.
  The default for this is ``FALSE``.

  .. versionadded:: 3.20

.. cmake:var:: OPENDDS_DEFAULT_SCOPE

  Default scope of unscoped files in :cmake:func:`opendds_target_sources`.
  It must be ``PRIVATE``, ``PUBLIC``, or ``INTERFACE``.
  The default for this is ``PRIVATE``.

  .. versionadded:: 3.20

.. cmake:var:: OPENDDS_AUTO_LINK_DCPS

  Automatically link ``OpenDDS::Dcps`` or other dependencies to the target of :cmake:func:`opendds_target_sources` using the "max" scope.
  The default for this is ``FALSE``, which means dependencies will have to be linked manually.

  .. note::

    This is off by default because it's not compatible with any existing usage of `target_link_libraries that doesn't specify a scope <https://cmake.org/cmake/help/latest/policy/CMP0023.html>`__.
    The default for this will be ``TRUE`` starting in OpenDDS 4.0.

  .. versionadded:: 3.24

.. cmake:var:: OPENDDS_USE_CORRECT_INCLUDE_SCOPE

  If ``TRUE``, then include directories of generated files using the "max" scope specified in :cmake:func:`opendds_target_sources`.
  The default for this is ``FALSE``, which always includes them using the ``PUBLIC`` scope.

  .. note::

    This is off by default because it could cause "Cannot find source file" errors on ``TypeSupport.idl`` files generated in a another directory.
    This will be fixed in OpenDDS 4.0 by requiring at least CMake 3.20 to get `CMP0118 <https://cmake.org/cmake/help/latest/policy/CMP0118.html>`__.
    This variable will be removed in OpenDDS 4.0 and the behavior will be the same as if this variable was set to ``TRUE``.

  .. versionadded:: 3.24

.. _cmake-config-vars:

Config Variables
-----------------

These variables are set by the configure script and normally shouldn't be changed.

Dependencies
^^^^^^^^^^^^

.. cmake:var:: OPENDDS_ACE

  Path to ACE, usually :envvar:`ACE_ROOT`

.. cmake:var:: OPENDDS_TAO

  Path to TAO, usually :envvar:`TAO_ROOT`

.. cmake:var:: OPENDDS_OPENSSL

  Path to OpenSSL

.. cmake:var:: OPENDDS_GTEST

  Path to Google Test

.. cmake:var:: OPENDDS_JAVA

  Path to Java

.. cmake:var:: OPENDDS_QT

  Path to QT

.. cmake:var:: OPENDDS_RAPIDJSON

  Path to RapidJSON

.. cmake:var:: OPENDDS_XERCES3

  Path to Xerces

.. _cmake-feature-vars:

Features
^^^^^^^^

.. cmake:var:: OPENDDS_CXX11

  ACE/TAO and OpenDDS were built with C++11 or later.
  Default depends on the compiler being used.

.. cmake:var:: OPENDDS_DEBUG

  Default is ``ON``

.. cmake:var:: OPENDDS_INLINE

  ``.inl`` files are included in header files.
  Default is ``ON``

.. cmake:var:: OPENDDS_VERSIONED_NAMEPSACE

  ACE/TAO and OpenDDS have versioned namespaces.
  Default is ``OFF``

.. cmake:var:: OPENDDS_STATIC

  ACE/TAO are built as static libraries.
  Default is ``OFF``

.. cmake:var:: OPENDDS_WCHAR

  ACE/TAO prefers using wide characters.
  Default is ``OFF``

.. cmake:var:: OPENDDS_TAO_IIOP

  Default is ``ON``

.. cmake:var:: OPENDDS_TAO_OPTIMIZE_COLLOCATED_INVOCATIONS

  Default is ``ON``

.. cmake:var:: OPENDDS_BUILT_IN_TOPICS

  Default is ``ON``

.. cmake:var:: OPENDDS_OBJECT_MODEL_PROFILE

  Default is ``ON``

.. cmake:var:: OPENDDS_PERSISTENCE_PROFILE

  Default is ``ON``

.. cmake:var:: OPENDDS_OWNERSHIP_PROFILE

  Default is ``ON``

.. cmake:var:: OPENDDS_OWNERSHIP_KIND_EXCLUSIVE

  Default is the value of :cmake:var:`OPENDDS_OWNERSHIP_PROFILE`.

.. cmake:var:: OPENDDS_CONTENT_SUBSCRIPTION

  Default is ``ON``

.. cmake:var:: OPENDDS_CONTENT_FILTERED_TOPIC

  Default is the value of :cmake:var:`OPENDDS_CONTENT_SUBSCRIPTION`.

.. cmake:var:: OPENDDS_MULTI_TOPIC

  Default is the value of :cmake:var:`OPENDDS_CONTENT_SUBSCRIPTION`.

.. cmake:var:: OPENDDS_QUERY_CONDITION

  Default is the value of :cmake:var:`OPENDDS_CONTENT_SUBSCRIPTION`.

.. cmake:var:: OPENDDS_SECURITY

  Default is ``OFF``

.. cmake:var:: OPENDDS_SAFETY_PROFILE

  Default is ``OFF``
