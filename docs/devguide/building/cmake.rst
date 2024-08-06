################################
Using OpenDDS in a CMake Project
################################

.. seealso::

  :ref:`cmake-building`

OpenDDS can be used with `CMake <https://cmake.org>`__\-based projects by using the :ghfile:`OpenDDS CMake config package <cmake>`.
This package bridges the gap between the MPC build system used by OpenDDS and CMake-based projects by providing :ref:`imported library targets <cmake-libraries>` and the ability to add IDL to a target using :cmake:func:`opendds_target_sources`.

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

  .. code:: bat

     cd DevGuideExamples\DCPS\Messenger
     mkdir build
     cd build
     cmake ..
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

By default the package will search for all libraries and executables, but only require the bare minimum for ACE/TAO and OpenDDS.
Arguments can be passed to ``find_package(OpenDDS COMPONENTS <argument>...)`` (or alternatively ``find_package(OpenDDS REQUIRED [COMPONENTS] <argument>...)``) add to what's required.
These can be:

- :ref:`Executables <cmake-executables>` and :ref:`Libraries <cmake-libraries>`

  Example:

  .. code-block:: cmake

    find_package(OpenDDS REQUIRED OpenDDS::Rtps_Udp OpenDDS::RtpsRelay)

- :ref:`Features <cmake-feature-vars>`

  Feature names should be the same as the variable, but without the leading ``OPENDDS_`` and all lowercase.
  Feature names by themselves will be required to be enabled.
  If using `CMake 3.9 or higher <https://cmake.org/cmake/help/latest/variable/CMAKE_MATCH_n.html>`__ and the name is followed by ``=``, then the `CMake boolean value <https://cmake.org/cmake/help/latest/command/if.html#basic-expressions>`__ following that determines if the feature must be enabled or disabled.

  Example:

  .. code-block:: cmake

    find_package(OpenDDS REQUIRED built_in_topics safety_profile=OFF)

  .. note::

    Passing features to ``OPTIONAL_COMPONENTS`` is treated as an error.
    It doesn't make sense to optionally request them because they are fixed.

- ``NO_DEFAULTS``

  Passing ``NO_DEFAULTS`` will only search for and require what is specified.
  This allows using ACE/TAO without OpenDDS being built, assuming the package is configured correctly.
  It also allows ``OPTIONAL_COMPONENTS`` to have an effect because normally everything is treated as optional.

  Example:

  .. code-block:: cmake

    find_package(OpenDDS REQUIRED NO_DEFAULTS ACE::ACE OPTIONAL_COMPONENTS TAO::TAO)

  This just makes :cmake:tgt:`ACE::ACE` and optionally :cmake:tgt:`TAO::TAO` available.
  Normally :cmake:tgt:`ACE::ACE`, :cmake:tgt:`TAO::TAO`, and :cmake:tgt:`OpenDDS::Dcps` are unconditionally searched for and required.

Adding IDL Sources with opendds_target_sources
==============================================

The CMake config package provides an easy way to add IDL sources to CMake targets using :cmake:func:`opendds_target_sources`.
Here is how it's used in the :ghfile:`Developer’s Guide Messenger example <DevGuideExamples/DCPS/Messenger/CMakeLists.txt>`:

.. literalinclude:: ../../../DevGuideExamples/DCPS/Messenger/CMakeLists.txt
  :language: cmake
  :start-at: IDL TypeSupport Library
  :end-before: set(opendds_libs

Here the IDL is added to a library that is shared by the executables, but ``opendds_target_sources`` can also be used on executables directly.

.. note:: CMake version 3.10 and below will issue a harmless warning if ``add_library`` is called without any sources.

See :cmake:func:`opendds_target_sources` for all the options it accepts.

Linking OpenDDS Libraries
=========================

In addition to C++ types generated from IDL and their type support, OpenDDS applications need discovery and transport to talk to each other.
`target_link_libraries <https://cmake.org/cmake/help/latest/command/target_link_libraries.html>`__ should be used with all the libraries needed.
Here is the usage in the :ghfile:`Developer’s Guide Messenger example <DevGuideExamples/DCPS/Messenger/CMakeLists.txt>`:

.. literalinclude:: ../../../DevGuideExamples/DCPS/Messenger/CMakeLists.txt
  :language: cmake
  :start-at: set(opendds_libs
  :end-at: target_link_libraries(subscriber ${opendds_libs})

See :ref:`cmake-libraries` for all the libraries the CMake package can provide.

.. _cmake-install-imported-runtime-artifacts:

install(IMPORTED_RUNTIME_ARTIFACTS)
===================================

.. versionadded:: 3.20

If using CMake 3.21 or later, it’s possible to install :ref:`executables <cmake-executables>` and :ref:`shared libraries <cmake-libraries>` from OpenDDS, ACE, and TAO in CMake along side the application using `install(IMPORTED_RUNTIME_ARTIFACTS) <https://cmake.org/cmake/help/latest/command/install.html#imported-runtime-artifacts>`__.
This will just install shared libraries and executables, not static libraries, headers, or anything else required for building applications.

If OpenDDS and ACE/TAO is built with ``clang``, the shared libraries might be missing an ``SONAME`` entry.
It is `an issue with ACE/TAO <https://github.com/DOCGroup/ACE_TAO/issues/1790>`__.
If trying to use ``install(IMPORTED_RUNTIME_ARTIFACTS)`` in this case, it causes the dynamic linker to ignore the libraries and report that they could not be found.
One workaround is to add ``SOFLAGS+=-Wl,-h,$(SONAME)`` to ``$ACE_ROOT/include/makeinclude/platform_macros.GNU`` before building.
This can be done manually after running the configure script or by passing ``--macros=SOFLAGS+=-Wl,-h,\$\(SONAME\)`` to the configure script.

:cmake:func:`opendds_get_library_dependencies` is provided to help find out what libraries need to be installed.

See the :ghfile:`install Test <tests/cmake/install/library/CMakeLists.txt>` for an example of using this.

.. versionchanged:: 3.25 There are now :ref:`executables <cmake-executables>` that can be installed.

.. _cmake-installing-generated-interface-files:

Installing Generated Interface Files
====================================

.. versionadded:: 3.20

It is possible to install files from the :ref:`OPENDDS_*_INTERFACE_FILES target properties <cmake-files-props>` for downstream projects to use.
See the :ghfile:`install Test <tests/cmake/install/library/CMakeLists.txt>` for an example of this.
It uses `install(FILES) <https://cmake.org/cmake/help/latest/command/install.html#files>`__, but there isn't any restriction on what installation method can be used.
For example, the `PUBLIC_HEADER <https://cmake.org/cmake/help/latest/prop_tgt/PUBLIC_HEADER.html>`__ target property could be set on target to the desired files from the interface lists.
Then they could be installed using `install(TARGETS ... PUBLIC_HEADER ...) <https://cmake.org/cmake/help/latest/command/install.html#targets>`__.
Another method is provided by :cmake:func:`opendds_install_interface_files`.

Manually Creating config.cmake
==============================

The :ghfile:`configure script <configure>` is responsible for generating the ``config.cmake`` file in :ghfile:`cmake`, which has various configuration options.
These options provide the OpenDDS CMake package with the required context it needs to integrate with the OpenDDS code generators and libraries.

If you are using OpenDDS libraries that were built without the help of the ``configure`` script, the ``config.cmake`` file needs to be created manually.
See :ref:`cmake-config-vars` for all the possible values to set.

*********
Reference
*********

Targets
=======

.. _cmake-libraries:

Libraries
---------

The CMake package can provide library targets that can be linked using `target_link_libraries <https://cmake.org/cmake/help/latest/command/target_link_libraries.html>`__ or installed using :ref:`cmake-install-imported-runtime-artifacts`.

.. cmake:tgt:: OpenDDS::Dcps

  Required, the core OpenDDS Library

.. cmake:tgt:: OpenDDS::Rtps

  :ref:`rtps-disc`

.. cmake:tgt:: OpenDDS::InfoRepoDiscovery

  :ref:`inforepo-disc`

.. cmake:tgt:: OpenDDS::Rtps_Udp

  :ref:`rtps-udp-transport`

.. cmake:tgt:: OpenDDS::Multicast

  :ref:`multicast-transport`

.. cmake:tgt:: OpenDDS::Shmem

  :ref:`shmem-transport`

.. cmake:tgt:: OpenDDS::Tcp

  :ref:`tcp-transport`

.. cmake:tgt:: OpenDDS::Udp

  :ref:`udp-transport`

.. cmake:tgt:: OpenDDS::Security

  :ref:`sec`

.. cmake:tgt:: OpenDDS::RtpsRelayLib

  Support library for :cmake:tgt:`OpenDDS::RtpsRelay`.

  .. versionadded:: 3.25

.. cmake:tgt:: ACE::ACE
  :no-contents-entry:

  Required

.. cmake:tgt:: ACE::XML_Utils
  :no-contents-entry:

.. cmake:tgt:: TAO::TAO
  :no-contents-entry:

  Required

.. cmake:tgt:: TAO::IDL_FE
  :no-contents-entry:

.. cmake:tgt:: TAO::AnyTypeCode
  :no-contents-entry:

.. cmake:tgt:: TAO::BiDirGIOP
  :no-contents-entry:

.. cmake:tgt:: TAO::CodecFactory
  :no-contents-entry:

.. cmake:tgt:: TAO::IORManip
  :no-contents-entry:

.. cmake:tgt:: TAO::IORTable
  :no-contents-entry:

.. cmake:tgt:: TAO::ImR_Client
  :no-contents-entry:

.. cmake:tgt:: TAO::PI
  :no-contents-entry:

.. cmake:tgt:: TAO::PortableServer
  :no-contents-entry:

.. cmake:tgt:: TAO::Svc_Utils
  :no-contents-entry:

.. cmake:tgt:: TAO::Valuetype
  :no-contents-entry:

.. _cmake-executables:

Executables
-----------

.. versionadded:: 3.25

The CMake package can provide executable targets that can be called manually from CMake or installed using :ref:`cmake-install-imported-runtime-artifacts`.

.. cmake:tgt:: ACE::ace_gperf

  Required

.. cmake:tgt:: TAO::tao_idl

  Required, :term:`tao_idl`

.. cmake:tgt:: OpenDDS::opendds_idl

  Required, :ref:`opendds_idl`

.. cmake:tgt:: OpenDDS::DCPSInfoRepo

  :ref:`inforepo`

.. cmake:tgt:: OpenDDS::RtpsRelay

  :ref:`rtpsrelay`

.. cmake:tgt:: OpenDDS::dcpsinfo_dump

  Utility for :ref:`inforepo`

.. cmake:tgt:: OpenDDS::inspect

  :ghfile:`tools/inspect/README.rst`

.. cmake:tgt:: OpenDDS::repoctl

  Utility for :ref:`inforepo`

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
      [USE_EXPORT <export-header>;<export-macro>]
      [USE_VERSIONED_NAMESPACE <vns-header>;<vns-prefix>]
      [GENERATE_SERVER_SKELETONS TRUE|FALSE]
      [AUTO_LINK TRUE|FALSE]
      [INCLUDE_BASE <dir>]
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

    If ``TRUE``, a header for exporting symbols in a shared library will be always generated as long the target is `some sort of library <https://cmake.org/cmake/help/latest/prop_tgt/TYPE.html>`__.
    If ``FALSE``, then it will only be done if the target is a shared library.
    The default is set by :cmake:var:`OPENDDS_ALWAYS_GENERATE_LIB_EXPORT_HEADER`.

    .. versionadded:: 3.20

  .. cmake:func:arg:: EXPORT_HEADER_DIR <header-dir>

    Where the export header is placed relative to the other generated files.
    The value is passed to :cmake:func:`opendds_export_header(DIR)`.

    .. versionadded:: 3.28

  .. cmake:func:arg:: USE_EXPORT <export-header>;<export-macro>

    Pass a CMake list (``;``-delimited) of an existing export header and export macro to use in the generated code.
    This is the same format as :cmake:func:`opendds_export_header(USE_EXPORT_VAR)`, but is intended for use with a custom export header.
    If there are multiple calls to :cmake:func:`opendds_target_sources` on the same target, only the first ``USE_EXPORT`` is used.

    .. versionadded:: 3.25

  .. cmake:func:arg:: USE_VERSIONED_NAMESPACE <version-ns-header>;<version-ns-prefix>

    Pass a CMake list (``;``-delimited) of an existing versioned namespace header and prefix to use in the generated code.

    .. versionadded:: 3.26

  .. cmake:func:arg:: GENERATE_SERVER_SKELETONS TRUE|FALSE

    ``tao_idl`` generate code for CORBA servers.
    The default is ``FALSE``.
    ``tao_idl`` by itself does this by default, but by default ``opendds_target_sources`` passes ``-SS`` to suppress this as it's not normally useful to an OpenDDS application.

    .. versionadded:: 3.25

  .. cmake:func:arg:: AUTO_LINK TRUE|FALSE

    Automatically link :cmake:tgt:`OpenDDS::Dcps` or other dependencies to the target using the "max" scope.
    If ``FALSE`` then dependencies will have to be linked manually.
    The default is set by :cmake:var:`OPENDDS_AUTO_LINK_DCPS`.

    .. versionadded:: 3.25

  .. cmake:func:arg:: INCLUDE_BASE <dir>

    Recreates the directory structure of all the passed IDLs relative to the passed base directory in the generated files.
    This allows using IDL files from multiple directories.
    :cmake:func:`opendds_install_interface_files` is proved to help install generated files that result from this.
    Any IDL file passed that's outside the include base will cause an error.

    The default behavior is the legacy behavior that assumes a flat hierarchy.
    Starting with OpenDDS 4.0 this will always be enabled and will default to ``CMAKE_CURRENT_SOURCE_DIR``.

    .. versionadded:: 3.26

  .. cmake:func:arg:: SKIP_TAO_IDL

    Skip invoking ``tao_idl`` on the IDL files that are passed in.
    This will still run ``tao_idl`` on ``*TypeSupport.idl`` files unless ``SKIP_OPENDDS_IDL`` is passed in or ``-SI`` is passed to :cmake:func:`opendds_target_sources(OPENDDS_IDL_OPTIONS)`.

    .. versionadded:: 3.25

  .. cmake:func:arg:: SKIP_OPENDDS_IDL

    Skip invoking ``opendds_idl`` on the IDL files.

    .. versionadded:: 3.25

  After ``opendds_target_sources`` is run on a target, it will have these target properties set on it:

  .. cmake:prop:: OPENDDS_LANGUAGE_MAPPINGS
    :no-contents-entry:

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

  .. _cmake-files-props:

  .. cmake:prop:: OPENDDS_GENERATED_DIRECTORY
    :no-contents-entry:

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
    :no-contents-entry:

    The ``PUBLIC`` and ``INTERFACE`` scoped IDL files passed.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_GENERATED_IDL_INTERFACE_FILES
    :no-contents-entry:

    The IDL files generated from the IDL files in :cmake:prop:`OPENDDS_PASSED_IDL_INTERFACE_FILES`.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_ALL_IDL_INTERFACE_FILES
    :no-contents-entry:

    Combination of :cmake:prop:`OPENDDS_PASSED_IDL_INTERFACE_FILES` and :cmake:prop:`OPENDDS_GENERATED_IDL_INTERFACE_FILES`.

  .. cmake:prop:: OPENDDS_GENERATED_HEADER_FILES
    :no-contents-entry:

    The ``.h`` and ``.inl`` files generated from :cmake:prop:`OPENDDS_ALL_IDL_INTERFACE_FILES`.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_ALL_GENERATED_INTERFACE_FILES
    :no-contents-entry:

    Combination of :cmake:prop:`OPENDDS_GENERATED_IDL_INTERFACE_FILES` and :cmake:prop:`OPENDDS_GENERATED_HEADER_FILES`.

    .. versionadded:: 3.20

  .. cmake:prop:: OPENDDS_ALL_INTERFACE_FILES
    :no-contents-entry:

    All the ``INTERFACE`` and ``PUBLIC`` scoped files that were passed in or generated.

    .. versionadded:: 3.20

  .. versionchanged:: 3.25

    ``OPENDDS_TARGET_SOURCES`` is now called ``opendds_target_sources``, but this shouldn't affect anything because functions and macros are case-insensitive in CMake.

.. cmake:func:: opendds_get_library_dependencies

  ::

    opendds_get_library_dependencies(<output-list-var-name> <target>...)

  If given targets provided by the CMake package, it will return a list of the targets along with their ACE, TAO, and OpenDDS dependencies.
  This is provided to help use :ref:`cmake-install-imported-runtime-artifacts`.

  .. versionadded:: 3.20

.. cmake:func:: opendds_export_header

  ::

    opendds_export_header(<target>
      [USE_EXPORT_VAR <use-export-var-name>]
      [DIR <dir-path>]
    )

  Generates a header that is compatible with `ACE's generate_export_file.pl <https://github.com/DOCGroup/ACE_TAO/blob/master/ACE/bin/generate_export_file.pl>`__ for exporting symbols in shared libraries.
  The header will able to be included as ``<target>_export.h`` and the macro that can be used to export symbols will be named ``<target>_Export``.
  It is the same function :cmake:func:`opendds_target_sources` uses so all the same info about :ref:`generated files <cmake-files-props>` applies.

  .. cmake:func:arg:: USE_EXPORT_VAR <use-export-var-name>

    Set a variable with the given name that contains a list with the location of the generated export header and the macro name to export a symbol.
    These values could be passed to :cmake:func:`opendds_target_sources(USE_EXPORT)`, but this shouldn't be necessary because these values are saved on the target on the first call of :cmake:func:`opendds_target_sources` or :cmake:func:`opendds_export_header`.

  .. cmake:func:arg:: DIR <dir-path>

    Where the export header is placed relative to the other generated files.
    By default the generated export header is put in the root of the include base.
    See :cmake:func:`opendds_target_sources(EXPORT_HEADER_DIR)` for how to pass this argument from there.

    .. versionadded:: 3.28 to replace the broken and undocumented ``INCLUDE_BASE`` argument.

  .. versionadded:: 3.25

.. cmake:func:: opendds_install_interface_files

  ::

    opendds_install_interface_files(<target>
      [DEST <dir>]
      [INCLUDE_BASE <dir>]
      [EXTRA_GENERATED_FILES <file>...]
    )

  A helper function that installs the files from the :ref:`OPENDDS_*_INTERFACE_FILES target properties <cmake-files-props>` of the given target and is meant to be used with :cmake:func:`opendds_target_sources(INCLUDE_BASE)`.
  The install path of a file ``idl_file`` could be described as ``DEST/relative(idl_file,INCLUDE_BASE)/idl_file``.

  .. cmake:func:arg:: DEST <dir>

    The base directory to install them to.
    By default this is `CMAKE_INSTALL_INCLUDEDIR <https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html>`__.

  .. cmake:func:arg:: INCLUDE_BASE <dir>

    The source directory relative to the source IDL files to define the directory structure of the installed files.
    This should probably be the same as :cmake:func:`opendds_target_sources(INCLUDE_BASE)`.
    By default this is ``CMAKE_CURRENT_SOURCE_DIR``.

  .. cmake:func:arg:: EXTRA_GENERATED_FILES <file>...

    Extra custom files that are in :cmake:prop:`OPENDDS_GENERATED_DIRECTORY` to install using the same method.

  .. versionadded:: 3.26

Variables
=========

Package Options
---------------

These variables can be used to override default behavior of the CMake package.

.. warning::

  Except for :cmake:var:`OPENDDS_CMAKE_VERBOSE`, do not set these when building OpenDDS using CMake.
  All the others will either have no effect or break the build.

.. cmake:var:: OPENDDS_CMAKE_VERBOSE

  If ``TRUE``, then log detailed status information at configure-time.
  The default for this is ``FALSE``.

  .. versionadded:: 3.25 The variable can also contain a list of categories to log more verbosely:

    ``all``
      Enables all logging

    ``components``
      Logs what :ref:`components <cmake-components>` were passed and the exact list of libraries are being searched for and required.

    ``imports``
      Logs the paths of :ref:`executables <cmake-executables>` and :ref:`libraries <cmake-libraries>` that are going to be imported.

    ``opendds_target_sources``
      Logs the arguments of :cmake:func:`opendds_target_sources` and what files will be generated.

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

  Setting this to ``TRUE`` tells ``opendds_idl`` to strip path information from ``#include`` lines in generated files.
  Turning the option on can make it easier to specify build rules for IDL files that include other IDL files.
  The default for this is ``FALSE``.

  .. versionadded:: 3.15

  .. deprecated:: 3.26

    :cmake:func:`opendds_target_sources(INCLUDE_BASE)` is a better way to handle IDL in multiple nested directories.

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

  Default value for :cmake:func:`opendds_target_sources(AUTO_LINK)`.
  The default for this is ``FALSE``.

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

These variables are set by the ``configure`` script in an MPC-built OpenDDS and normally shouldn't be changed.
They can be changed when configuring a :ref:`CMake-built OpenDDS <cmake-building>` using ``-D``, but should not be changed after that.

.. seealso::

  :ref:`cmake-building-vars`

Dependencies
^^^^^^^^^^^^

.. cmake:var:: OPENDDS_ACE
  :no-contents-entry:

  Path to :ref:`deps-ace`, usually :envvar:`ACE_ROOT`

.. cmake:var:: OPENDDS_ACE_VERSION
  :no-contents-entry:

  The version of ACE being used.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_TAO
  :no-contents-entry:

  Path to :ref:`deps-tao`, usually :envvar:`TAO_ROOT`

.. cmake:var:: OPENDDS_TAO_VERSION
  :no-contents-entry:

  The version of TAO being used.

  .. versionadded:: 3.27

.. cmake:var:: OPENDDS_OPENSSL
  :no-contents-entry:

  Path to :ref:`deps-openssl`

.. cmake:var:: OPENDDS_GTEST
  :no-contents-entry:

  Path to :ref:`deps-gtest`

.. cmake:var:: OPENDDS_JAVA
  :no-contents-entry:

  Path to :ref:`deps-java`
  Currently unsupported.

.. cmake:var:: OPENDDS_QT
  :no-contents-entry:

  Path to :ref:`deps-qt`

.. cmake:var:: OPENDDS_RAPIDJSON
  :no-contents-entry:

  Path to :ref:`deps-rapidjson`

.. cmake:var:: OPENDDS_XERCES3
  :no-contents-entry:

  Path to :ref:`deps-xerces`

.. cmake:var:: OPENDDS_HOST_TOOLS

  A directory that contains a ``bin`` directory with ``opendds_idl`` to be used for cross-compiling.

  .. versionadded:: 3.26

.. cmake:var:: OPENDDS_ACE_TAO_HOST_TOOLS

  A directory that contains a ``bin`` directory with ``tao_idl`` and ``ace_gperf`` to be used for cross-compiling.
  This isn't set by default unless :cmake:var:`OPENDDS_HOST_TOOLS` is set, in which case it defaults to that.

  .. versionadded:: 3.26

.. _cmake-feature-vars:

Features
^^^^^^^^

.. cmake:var:: OPENDDS_CXX11
  :no-contents-entry:

  ACE/TAO and OpenDDS were built with C++11 or later.
  Default depends on the compiler being used.
  Has no effect when building OpenDDS using CMake.

.. cmake:var:: OPENDDS_CXX_STD

  .. versionadded:: 3.28

  If defined, then this value is prefixed with ``cxx_std_`` and passed to `target_compile_features <https://cmake.org/cmake/help/latest/command/target_compile_features.html>`__ for all targets.
  The valid values are the standards in `CMAKE_CXX_KNOWN_FEATURES <https://cmake.org/cmake/help/latest/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#high-level-meta-features-indicating-c-standard-support>`__ without the leading ``cxx_std_``.
  This is the minimum C++ standard required to use the ACE, TAO, and OpenDDS libraries.
  When building OpenDDS using CMake, it's also the minimum C++ standard used to build OpenDDS.
  The default depends on the version of ACE being used.

.. cmake:var:: OPENDDS_DEBUG
  :no-contents-entry:

  Default depends on `CMAKE_BUILD_TYPE <https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html>`__.
  When building OpenDDS using CMake ``CMAKE_BUILD_TYPE`` should be used.

.. cmake:var:: OPENDDS_OPTIMIZE
  :no-contents-entry:

  Default depends on `CMAKE_BUILD_TYPE <https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html>`__.
  When building OpenDDS using CMake ``CMAKE_BUILD_TYPE`` should be used.

.. cmake:var:: OPENDDS_INLINE
  :no-contents-entry:

  ``.inl`` files are included in header files.
  Default is ``ON``
  Has no effect when building OpenDDS using CMake.

.. cmake:var:: OPENDDS_VERSIONED_NAMESPACE
  :no-contents-entry:

  ACE/TAO and OpenDDS have versioned namespaces.
  Default is ``OFF``

.. cmake:var:: OPENDDS_STATIC
  :no-contents-entry:

  ACE/TAO are built as static libraries.
  Default depends on `BUILD_SHARED_LIBS <https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html>`__.
  When building OpenDDS using CMake ``BUILD_SHARED_LIBS`` should be used.

.. cmake:var:: OPENDDS_WCHAR
  :no-contents-entry:

  ACE/TAO prefers using wide characters.
  Default is ``OFF``

.. cmake:var:: OPENDDS_TAO_IIOP
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_TAO_OPTIMIZE_COLLOCATED_INVOCATIONS
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_BUILT_IN_TOPICS
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_OBJECT_MODEL_PROFILE
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_PERSISTENCE_PROFILE
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_OWNERSHIP_PROFILE
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_OWNERSHIP_KIND_EXCLUSIVE
  :no-contents-entry:

  Default is the value of :cmake:var:`OPENDDS_OWNERSHIP_PROFILE`.

.. cmake:var:: OPENDDS_CONTENT_SUBSCRIPTION
  :no-contents-entry:

  Default is ``ON``

.. cmake:var:: OPENDDS_CONTENT_FILTERED_TOPIC
  :no-contents-entry:

  Default is the value of :cmake:var:`OPENDDS_CONTENT_SUBSCRIPTION`.

.. cmake:var:: OPENDDS_MULTI_TOPIC
  :no-contents-entry:

  Default is the value of :cmake:var:`OPENDDS_CONTENT_SUBSCRIPTION`.

.. cmake:var:: OPENDDS_QUERY_CONDITION
  :no-contents-entry:

  Default is the value of :cmake:var:`OPENDDS_CONTENT_SUBSCRIPTION`.

.. cmake:var:: OPENDDS_SECURITY
  :no-contents-entry:

  Default is ``OFF``

.. cmake:var:: OPENDDS_SAFETY_PROFILE
  :no-contents-entry:

  Default is ``OFF``
