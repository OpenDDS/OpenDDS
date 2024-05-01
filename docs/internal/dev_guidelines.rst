##############################
OpenDDS Development Guidelines
##############################

This document organizes our current thoughts around development guidelines in a place that's readable and editable by the overall user and maintainer community.
It's expected to evolve as different maintainers get a chance to review and contribute to it.

Although ideally all code in the repository would already follow these guidelines, in reality the code has evolved over many years by a diverse group of developers.
At one point an automated re-formatter was run on the codebase, migrating from the `GNU C style <https://www.gnu.org/prep/standards/html_node/Writing-C.html>`_ to the current, more conventional style, but automated tools can only cover a subset of the guidelines.

**********
Repository
**********

The repository is hosted on Github at `OpenDDS/OpenDDS <https://github.com/OpenDDS/OpenDDS>`_ and is open for pull requests.

*******
Testing
*******

If a pull request fixes a bug that's not covered in an existing test it should be added to the tests.
This should be in an existing test if possible.
If a new integration test is required, see :ghfile:`tests/DCPS/HelloWorld` for a template.
Pull requests will be tested automatically using `GitHub Actions <https://github.com/OpenDDS/OpenDDS/actions>`__.

.. seealso::

  :doc:`running_tests` for how tests are run in general.

  :doc:`unit_tests` for guidance on the unit tests.

  :doc:`github_actions` for how building and testing is done with GitHub Actions.

************
Dependencies
************

* MPC is the build system, used to configure the build and generate platform specific build files (Makefiles, VS solution files, etc.).
* ACE is a library used for cross-platform compatibility, especially networking and event loops.
  It is used both directly and through TAO.
* TAO is a C++ CORBA implementation built on ACE.

  * It's used to communicate with DCPSInfoRepo, which is one option for Discovery.
  * TAO's data types and support for the OMG IDL-to-C++ mapping are also used in the End User DDS API.
  * The TAO IDL compiler is used internally and by the end user to allow OpenDDS to use user-defined IDL types as topic data.

* Perl is an interpreted language used in the configure script, the tests, and any other scripting in OpenDDS codebase.
* Google Test is required for OpenDDS tests.
  By default, CMake will be used to build a specific version of Google Test that we have as a submodule.
  An appropriate prebuilt or system Google Test can also be used.

.. seealso::

  :doc:`/devguide/building/dependencies` for all dependencies and details on how these are used in OpenDDS.

*********************
Dependency Management
*********************

ACE/TAO
=======

By default, the :ghfile:`configure` script and :ref:`CMake support<cmake-building>` use released versions of ACE/TAO.
The same versions are used for pull requests.
The versions are documented in :ghfile:`acetao.ini`.
Currently, the :ghfile:`configure` scripts uses a release of ACE6 and CMake uses a release of ACE7.

The versions are updated by the GitHub Actions workflow in :ghfile:`.github/workflows/update-ace-tao.yml` for micro/patch releases.
When a new version of ACE/TAO is available, the workflow creates a pull request based on the new version.
Ideally, this pull request would be merged early in a development cycle to allow the maximum number of builds to use the new versions.
Changing the major and minor version will be considered at the beginning of a development cycle.

To stay abreast of changes in ACE/TAO and provide feedback to ACE/TAO, there are additional GitHub Action workflows for build and testing with various ACE/TAO branches.
These workflows are executed on a periodic basis.

A developer is allowed to change the ACE/TAO version for their PR.
If this is not a release, then the PR would be merged after the necessary ACE/TAO functionality is released.
Ideally, the GitHub Actions files would be extended with variables that allow the default versions to be changed easily and a lint script would be added to prevent unintended changes.

TODO
====

* MPC
* Perl
* openssl
* xerces
* rapidjson
* GoogleTest
* vcpkg

.. _dev_guidelines-text_file_formating:

********************
Text File Formatting
********************

All text files in the source code repository follow a few basic rules.
These apply to C++ source code, Perl scripts, MPC files, and any other plaintext file.

* A text file is a sequence of lines, each ending in the "end-of-line" character (AKA Unix line endings).
* Based on this rule, all files end with the end-of-line character.
* The character before end-of-line is a non-whitespace character (no trailing whitespace).
* Tabs are not used.

  * One exception, MPC files may contain literal text that's inserted into Makefiles which could require tabs.
  * In place of a tab, use a set number of spaces, depending on what type of file it is:

    * C++ and everywhere else unless otherwise noted should always be 2 spaces.
    * Perl is usually 2 spaces, but some files are defiant and use 4 spaces.
      See :ref:`dev_guidelines-perl_coding_style` for details.
    * Python should always be 4 spaces.
      See :ref:`dev_guidelines-python_coding_style` for details.

* Keep line length reasonable.
  I don't think it makes sense to strictly enforce an 80-column limit, but overly long lines are harder to read.
  Try to keep lines to roughly 80 characters.

The :ref:`lint script <dev_guidelines-lint-script>` will help check for most of these in a PR.

There is also a :ghfile:`.editorconfig` file that allows contributors to follow most of these rules automatically.
`EditorConfig <https://editorconfig.org/>`__ support is `built-in to some editors (including Visual Studio) <https://editorconfig.org/#pre-installed>`__ with `plugins available for others <https://editorconfig.org/#download>`__.

.. _dev_guidelines-lint-script:

***********
Lint Script
***********

The :ghfile:`lint script <tools/scripts/lint.pl>` is a Perl script that is run on every PR.
It checks for mistakes in both coding and style.
It can also be run locally to check for issues before committing.
If it is ran without arguments it does the default set of checks and also runs ACE's ``fuzz.pl`` if available.
To see a list of the default checks with descriptions, run the script with ``--list``.
Passing ``--try-fix`` will try to fix some of those issues.
The script also has ways to skip some or all checks for single lines or whole files.
Pass ``--help`` for more information.

*************
Documentation
*************

Guidelines for building and editing documentation like the Developer's Guide and this document are covered in :doc:`docs`.

If a pull request makes a change that should be included in the release notes, the entry should be specified using the method described in :ref:`docs-news`.

.. _dev_guidelines-cxx_standard:

************
C++ Standard
************

The base C++ standard used in OpenDDS is C++03.
There are some optional features that are only built when a newer C++ standard level is used.
See uses of the MPC feature ``no_cxx11`` and the base project :ghfile:`MPC/config/opendds_cxx11.mpb`.

Avoid using implementation-defined extensions (including ``#pragma``). Exceptions are:

* ``#pragma once`` which only impacts preprocessing and is understood across all supported compilers, or harmlessly ignored if not understood
* ``#pragma pack`` can only be used on POD structs to influence alignment/padding

Use the C++ standard library as much as possible.
The standard library should be preferred over ACE, which in turn should be preferred over system-specific libraries.

The C++ standard library includes the C standard library by reference, making those identifiers available in namespace ``std``.
Using C's standard library identifiers in namespace ``std`` is preferred over the global namespace -- ``#include <cstring>`` instead of ``#include <string.h>``.
Not all supported platforms have standard library support for wide characters (``wchar_t``) but this is rarely needed.
Preprocessor macro ``DDS_HAS_WCHAR`` can be used to detect those platforms.

****************
C++ Coding Style
****************

* C++ code in OpenDDS must compile under the :ghfile:`compilers listed in the \`\`README.md\`\` file <README.md#compilers>`.
* Commit code in the proper style from the start, so follow-on commits to adjust style don't clutter history.
* C++ source code is a plaintext file, so the guidelines in :ref:`dev_guidelines-text_file_formating` apply.
* A modified Stroustrup style is used (see :ghfile:`tools/scripts/style`).

  * Warning: not everything in :ghfile:`tools/scripts/style` represents the current guidelines.

* Sometimes the punctuation characters are given different names, this document will use:

  * Parentheses ``( )``
  * Braces ``{ }``
  * Brackets ``[ ]``

Example
=======

.. code-block:: C++

   template<typename T>
   class MyClass : public Base1, public Base2 {
   public:
     bool method(const OtherClass& parameter, int idx = 0) const;
   };

   template<typename T>
   bool MyClass<T>::method(const OtherClass& parameter, int) const
   {
     if (parameter.foo() > 42) {
       return member_data_;
     } else {
       for (int i = 0; i < some_member_; ++i) {
         other_method(i);
       }
       return false;
     }
   }

Punctuation
===========

The punctuation placement rules can be summarized as:

* Open brace appears as the first non-whitespace character on the line to start function definitions.
* Otherwise the open brace shares the line with the preceding text.
* Parentheses used for control-flow keywords (``if``, ``while``, ``for``, ``switch``) are separated from the keyword by a single space.
* Otherwise parentheses and brackets are not preceded by spaces.

Whitespace
==========

* Each "tab stop" is two spaces.
* Namespace scopes that span most or all of a file do not cause indentation of their contents.
* Otherwise lines ending in ``{`` indicate that subsequent lines should be indented one more level until ``}``.
* Continuation lines (when a statement spans more than one line) can either be indented one more level, or indented to nest "under" an ``(`` or similar punctuation.
* Add space around binary operators and after commas: ``a + b, c``
* Do not add space around parentheses for function calls, a properly formatted function call looks like ``func(arg1, arg2, arg3);``
* Do not add space around brackets for indexing, instead it should look like: ``mymap[key]``
* For code that includes multiple braces appearing together in the same expression (such as initializer lists), there are two approved styles:

  * spaces between braces and their enclosed (non-empty) sub-expression: ``const GUID_t GUID_UNKNOWN = { { 0 }, { { 0 }, 0 } };`` or ``{ a + b, {} }``
  * no such spaces: ``const GUID_t GUID_UNKNOWN = {{0}, {{0}, 0}};`` or ``{a + b, {}}``

* Do not add extra spaces to make syntax elements (that span lines/statements) line up; this only causes unnecessary changes in adjacent lines as the code evolves.
* In general, do not add extra spaces unless doing so is covered by the rules above.

Language Usage
==============

* Add braces following control-flow keywords even when they are optional.
* ``this->`` is not used unless required for disambiguation or to access members of a template-dependent base class.
* Declare local variables at the latest point possible.
* ``const`` is a powerful tool that enables the compiler to help the programmer find bugs.
  Use ``const`` everywhere possible, including local variables.
* Modifiers like ``const`` appear left of the types they modify, like: ``const char* cstring = ...``.
  ``char const*`` is equivalent but not conventional.
* For function arguments that are not modified by the callee, pass by value for small objects (8 bytes?) and pass by const-reference for everything else.
  Function argument that is passed by value should not have ``const`` qualifier in the function declaration; use of ``const`` in the definition is optional.
* Arguments unused by the implementation have no names (in the definition that is, the declarations still have names), or a ``/*commented-out*/`` name.
* Use ``explicit`` constructors unless implicit conversions are intended and desirable.
* Use the constructor initializer list and make sure its order matches the declaration order.
* Prefer pre-increment/decrement (``++x``) to post-increment/decrement (``x++``) for both objects and non-objects.
* All currently supported compilers use the template inclusion mechanism.
  Thus function/method template definitions may not be placed in normal ``*.cpp`` files, instead they can go in ``_T.cpp`` (which are ``#included`` and not separately compiled), or directly in the ``*.h``.
  In this case, ``*_T.cpp`` takes the place of ``*.inl`` (except it is always inlined).
  See ACE for a description of ``*.inl`` files.

Pointers and References
=======================

Pointers and references go along with the type, not the identifier.
For example:

.. code-block:: C++

   int* intPtr = &someInt;

Watch out for multiple declarations in one statement.
``int* c, b;`` does not declare two pointers! It's best just to break these into separate statements:

.. code-block:: C++

   int* c;
   int* b;

In code targeting C++03, ``0`` should be used as the null pointer.
For C++11 and later, ``nullptr`` should be used instead.
``NULL`` should never be used.

Naming
======

**(For library code that the user may link to)**

* Preprocessor macros visible to user code must begin with ``OPENDDS_``
* C++ identifiers are either in top-level namespace ``DDS`` (OMG spec defined) or ``OpenDDS`` (otherwise)
* Within the ``OpenDDS`` namespace there are some nested namespaces:

  * ``DCPS``: anything relating to the implementation of the DCPS portion of the DDS spec
  * ``RTPS``: types directly tied to the RTPS spec
  * ``Federator``: DCPSInfoRepo federation
  * ``FileSystemStorage``: reusable component for persistent storage

* Naming conventions

  * ``ClassesAreCamelCaseWithInitialCapital``
  * ``methodsAreCamelCaseWithInitialLower`` OR ``methods_are_lower_case_with_underscores``
  * ``member_data_use_underscores_and_end_with_an_underscore_``
  * ``ThisIsANamespaceScopedOrStaticClassMemberConstant``

.. note::

  For CMake `<https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html>` are nominally supported.
  This may cause unexpected build issues in CI builds when a name in one file happens to clash with another source file in the same source file batch.

Comments
========

* Add comments only when they will provide MORE information to the reader.
* Describing the code verbatim in comments doesn't add any additional information.
* If you start out implementation with comments describing what the code will do (or pseudocode), review all comments after implementation is done to make sure they are not redundant.
* Do not add a comment before the constructor that says ``// Constructor``.
  We know it's a constructor.
  The same note applies to any redundant comment.

.. _dev_guidelines-documenting_code_for_doxygen:

Documenting Code for Doxygen
============================

This is a simple guide that shows how to use Doxygen in OpenDDS.

.. seealso:: `The Doxygen manual <https://www.doxygen.nl/manual/>`_ for a complete guide to using Doxygen.

Doxygen supports multiple styles of documenting comments but this style should be used in non-trivial situations:

.. code-block:: C++

   /**
    * This sentence is the brief description.
    *
    * Everything else is the details.
    */
   class DoesStuff {
   // ...
   };

For simple things, a single line documenting comment can be made like:

.. code-block:: C++

   /// Number of bugs in the code
   unsigned bug_count = -1; // Woops

The extra ``*`` on the multiline comment and ``/`` on the single line comment are important.
They inform Doxygen that comment is the documentation for the following declaration.

If referring to something that happens to be a namespace or other global object (like DDS, OpenDDS, or RTPS), you should precede it with a ``%``.
If not it will turn into a link to that object.

Preprocessor
============

* If possible, use other language features things like inlining and constants instead of the preprocessor.
* Prefer ``#ifdef`` and ``#ifndef`` to ``#if defined`` and ``#if !defined`` when testing if a single macro is defined.
* Leave parentheses off preprocessor operators.  For example, use ``#if defined X && defined Y`` instead of ``#if defined(X) && defined(Y)``.
* As stated before, preprocessor macros visible to user code must begin with ``OPENDDS_``.
* See section :ref:`dev_guidelines-cxx_standard` above for notes on ``#pragma``.
* Ignoring the header guard if there is one, preprocessor statements should be indented using two spaces starting at the pound symbol, like so:

.. code-block:: C++

   #if defined X && defined Y
   #  if X > Y
   #    define Z 1
   #  else
   #    define Z 0
   #  endif
   #else
   #  define Z -1
   #endif

Includes
--------

Order
^^^^^

As a safeguard against headers being dependant on a particular order, includes should be ordered based on a hierarchy going from local headers to system headers, with spaces between groups of includes.
Generated headers from the same directory should be placed last within these groups.
This order can be generalized as the following:

1. Pre-compiled header if it is required for a ``.cpp`` file by Visual Studio.
2. The corresponding header to the source file (``Foo.h`` if we were in ``Foo.cpp``).
3. Headers from the local project.
4. Headers from external OpenDDS-based libraries.
5. Headers from :ghfile:`dds/DCPS`.
6. ``dds/*C.h`` Headers
7. Headers from external TAO-based libraries.
8. Headers from TAO.
9. Headers from external ACE-based libraries.
10. Headers from ACE.
11. Headers from external non-ACE-based libraries.
12. Headers from system and C++ standard libraries.

There can be exceptions to this list.
For example if a header from ACE or the system library was needed to decide if another header should be included.

Path
^^^^

Headers should only use local includes (``#include "foo/Foo.h"``) if the header is relative to the file.
Otherwise system includes (``#include <foo/Foo.h>``) should be used to make it clear that the header is on the system include path.

In addition to this, includes for a file that will always be relative to the including file should have a relative include path.
For example, a ``dds/DCPS/bar.cpp`` should include ``dds/DCPS/bar.h`` using ``#include "bar.h"``, not ``#include <dds/DCPS/bar.h>`` and especially not ``#include "dds/DCPS/bar.h"``.

Example
^^^^^^^

For a ``Doodad.cpp`` file in :ghfile:`dds/DCPS`, the includes could look like:

.. code-block:: C++

  #include <DCPS/DdsDcps_pch.h>

  #include "Doodad.h"

  #include <ace/config-lite.h>
  #ifndef ACE_CPP11
  #  include "ConditionVariable.h"
  #endif
  #include "ReactorTask.h"
  #include "transport/framework/DataLink.h"

  #include <dds/DdsDcpsCoreC.h>

  #include <tao/Version.h>

  #include <ace/Version.h>

  #include <openssl/opensslv.h>

  #include <unistd.h>
  #include <stdlib.h>

Initialization
==============

Note that OpenDDS applications require ACE to be initialized to work correctly. For many OpenDDS applications, ``ACE::init()`` and ``ACE::fini()`` will be called
automatically, either by interaction with the ACE or TAO libraries, or due to ACE's redefinition of executable entry points (e.g. ``main``) which wrap normal execution
with calls to those functions. However, be advised that on some platforms, the helper macros to catch entry points may change names to suit compiler options. For example,
for Visual C++ builds on Windows with wide-character support enabled, the helper macro changes from ``main`` to ``wmain``. Applications either need to handle these differences
in order to correctly ensure initialization or they need to use an entrypoint helper macro such as ``ACE_TMAIN`` which isn't vulnerable to this issue.

Time
====

Measurements of time can be broken down into two basic classes: A specific point in time (Ex: 00:00 January 1, 1970) and a length or duration of time without context (Ex: 134 Seconds).
In addition, a computer can change its clock while a program is running, which could mess up any time lapses being measured.
To solve this problem, operating systems provide what's called a monotonic clock that runs independently of the normal system clock.

ACE can provide monotonic clock time and has a class for handling time measurements, ``ACE_Time_Value``, but it doesn't differentiate between specific points in time and durations of time.
It can differentiate between the system clock and the monotonic clock, but it does so poorly.
OpenDDS provides three classes that wrap ``ACE_Time_Value`` to fill these roles: ``TimeDuration``, ``MonotonicTimePoint``, and ``SystemTimePoint``.
All three can be included using :ghfile:`dds/DCPS/TimeTypes.h`.
Using ``ACE_Time_Value`` is discouraged unless directly dealing with ACE code which requires it and using ``ACE_OS::gettimeofday()`` or ``ACE_Time_Value().now()`` in C++ code in :ghfile:`dds/DCPS` treated as an error by the :ref:`lint script <dev_guidelines-lint-script>`.

``MonotonicTimePoint`` should be used when tracking time elapsed internally and when dealing with ``ACE_Time_Value``\s being given by the ``ACE_Reactor`` in OpenDDS.
``ACE_Condition``\s, like all ACE code, will default to using system time.
Therefore the ``Condition`` class wraps it and makes it so it always uses monotonic time like it should.
Like ``ACE_OS::gettimeofday()``, referencing ``ACE_Condition`` in :ghfile:`dds/DCPS` will be treated as an error by the :ref:`lint script <dev_guidelines-lint-script>`.

More information on using monotonic time with ACE can be found `here <https://htmlpreview.github.io/?https://github.com/DOCGroup/ACE_TAO/blob/master/ACE/docs/ACE-monotonic-timer.html>`_.

``SystemTimePoint`` should be used when dealing with the DDS API and timestamps on incoming and outgoing messages.

Logging
=======

ACE Logging
-----------

Logging is done via ACE's logging macro functions, ``ACE_DEBUG`` and ``ACE_ERROR``, defined in ``ace/Log_Msg.h``.
The logging macros arguments to both are:

- A ``ACE_Log_Priority`` value

  - This is an enum defined in ``ace/Log_Priority.h`` to say what the priority or severity of the message is.

- The format string

  - This is similar to the format string for the standard ``printf``, where it substitutes sequences starting with ``%``, but the format of theses sequences is different.
    For example ``char*`` values are substituted using ``%C`` instead of ``%s``.
    See the documenting comment for ``ACE_Log_Msg::log`` in ``ace/Log_Msg.h`` for what the format of the string is.

- The variable number of arguments

  - Like ``printf`` the variable arguments can't be whole objects, like a ``std::string`` value.
    In the case of ``std::string``, the format and arguments would look like: ``"%C", a_string.c_str()``.

Note that all the ``ACE_DEBUG`` and ``ACE_ERROR`` arguments must be surrounded by two sets of parentheses.

.. code-block:: C++

  ACE_DEBUG((LM_DEBUG, "Hello, %C!\n", "world"));

ACE logs to ``stderr`` by default on conventional platforms, but can log to other places.

Usage in OpenDDS
----------------

Logging Conditions and Priority
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In OpenDDS ``ACE_DEBUG`` and ``ACE_ERROR`` are used directly most of the time, but sometimes they are used indirectly, like with the transport framework's ``VDBG`` and ``VDBG_LVL``.
They also should be conditional on one of the logging control systems in OpenDDS.

.. seealso:: See :ref:`run_time_configuration--logging` for the user perspective.

The logging conditions are as follows:

+--------------------------------+---------------+----------------+------------------------------------+
| Message Kind                   | Macro         | Priority       | Condition                          |
+================================+===============+================+====================================+
| Unrecoverable error            | ``ACE_ERROR`` | ``LM_ERROR``   | ``log_level >= LogLevel::Error``   |
+--------------------------------+---------------+----------------+------------------------------------+
| Unreportable recoverable error | ``ACE_ERROR`` | ``LM_WARNING`` | ``log_level >= LogLevel::Warning`` |
+--------------------------------+---------------+----------------+------------------------------------+
| Reportable recoverable error   | ``ACE_ERROR`` | ``LM_NOTICE``  | ``log_level >= LogLevel::Notice``  |
+--------------------------------+---------------+----------------+------------------------------------+
| Informational message          | ``ACE_DEBUG`` | ``LM_INFO``    | ``log_level >= LogLevel::Info``    |
+--------------------------------+---------------+----------------+------------------------------------+
| Debug message                  | ``ACE_DEBUG`` | ``LM_DEBUG``   | Based on ``DCPS_debug_level`` or   |
|                                |               |                | one of the other debug systems     |
|                                |               |                | :ref:`listed below <dbg-lvl-sys>`  |
|                                |               |                | [#lldbg]_                          |
+--------------------------------+---------------+----------------+------------------------------------+

An `unrecoverable error` indicates that OpenDDS is in a state where it cannot function as intended.
This may be the result of a defect, misconfiguration, or interference.

A `recoverable error` indicates that OpenDDS could not perform a desired action but remains in a state where it can function as intended.

A `reportable error` indicates that OpenDDS can report the error via the API through something like an exception or return value.

An `informational message` gives high level information mostly at startup, like the version of OpenDDS being used.

A `debug message` gives lower level information, such as if a message is being sent.
These are directly controlled by one of a few debug logging control systems.

.. _dbg-lvl-sys:

- ``DCPS_debug_level`` should be used for all debug logging that doesn't fall under the other systems.
  It is an unsigned integer value which ranges from 0 to 10.
  See :ghfile:`dds/DCPS/debug.h` for details.

- ``Transport_debug_level`` should be used in the transport layer.
  It is an unsigned integer value which ranges from 0 to 6.
  See :ghfile:`dds/DCPS/transport/framework/TransportDebug.h` for details.

- ``security_debug`` should be used for logging in related to DDS Security.
  It is an object with ``bool`` members that make up categories of logging messages that allow fine control.
  See :ghfile:`dds/DCPS/debug.h` for details.

For number-based conditions like ``DCPS_debug_level`` and ``Transport_debug_level``, the number used should be the log level the message starts to become active at.
For example for ``DCPS_debug_level >= 6`` should be used instead of ``DCPS_debug_level > 5``.

.. [#lldbg] Debug messages don't rely on both `LogLevel::Debug` and a debug control system.
  The reason is that it results in a simpler check and the log level already loosely controls all the debug control systems.
  See the ``LogLevel::set`` function in :ghfile:`dds/DCPS/debug.cpp` for exactly what it does.

Message Content
^^^^^^^^^^^^^^^

- Log messages should take the form::

    (%P|%t) [ERROR:|WARNING:|NOTICE:|INFO:] FUNCTION_NAME: MESSAGE\n

  - Use ``ERROR:``, ``WARNING:``, ``NOTICE:``, and ``INFO:`` if using the corresponding log priorities.
  - ``CLASS_NAME::METHOD_NAME`` should be used instead of just the function name if it's part of a class.
    It's at the developer's discretion to come up with a meaningful name for members of overload sets, templates, and other more complex cases.
  - ``security_debug`` and ``transport_debug`` log messages should indicate the category name, for example:

    .. code-block:: C++

      if (security_debug.access_error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: {access_error} example_function: Hello, World!\n"));
      }

- Format strings should not be wrapped in ``ACE_TEXT``.
  We shouldn't go out of our way to replace it in existing logging points, but it should be avoided it in new ones.

  - ``ACE_TEXT``'s purpose is to wrap strings and characters in ``L`` on builds where ``uses_wchar=1``, so they become the wide versions.
  - While not doing it might result in a performance hit for character encoding conversion at runtime, the builds where this happens are rare, so it's outweighed by the added visual noise to the code and the possibility of bugs introduced by improper use of ``ACE_TEXT``.

- Avoid new usage of ``ACE_ERROR_RETURN`` in order to not hide the return statement within a macro.

Examples
^^^^^^^^

.. code-block:: C++

  if (log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: example_function: Hello, World!\n"));
  }

  if (log_level >= LogLevel::Warning) {
    ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: example_function: Hello, World!\n"));
  }

  if (log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: example_function: Hello, World!\n"));
  }

  if (log_level >= LogLevel::Info) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: example_function: Hello, World!\n"));
  }

  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) example_function: Hello, World!\n"));
  }

.. _dev_guidelines-perl_coding_style:

*****************
Perl Coding Style
*****************

`The Perl style guide <https://perldoc.perl.org/perlstyle>`_ should be generally followed, as long as it doesn't conflict with :ref:`dev_guidelines-text_file_formating`.
Some additional notes and exceptions:

- New files should use 2 space indents, while existing 4 space indent files should stay that way for the most part.

- The style of ``if``/``elsif``/``else`` should be this:

  .. code-block:: perl

    if (x) {
    }
    elsif (y) {
    }
    else {
    }

  This is most likely what the Perl style guide refers to when it says "Uncuddled elses".

- Prefer calling functions with parentheses around the arguments where possible.

- The Perl style guide says to add spaces to line things up across multiple lines, but do not do this.
  The reason is the same as in C++ and that is that it reduces the flexibility of the code.

- Put the following at the start of a Perl file as soon as possible:

  .. code-block:: perl

    use strict;
    use warnings;

  They should go before the imports, so that they can help reveal as many problems as possible.

.. _dev_guidelines-python_coding_style:

*******************
Python Coding Style
*******************

In the world of Python usage of some form of :pep:`8` is basically universal.
It should be generally followed, including indents being 4 spaces, as long as it doesn't conflict with :ref:`dev_guidelines-text_file_formating`.

******************
CMake Coding Style
******************

`The vcpkg CMake style guide <https://learn.microsoft.com/en-us/vcpkg/contributing/cmake-guidelines>`_ should be generally followed, as long as it doesn't conflict with :ref:`dev_guidelines-text_file_formating`.
Some additional notes and exceptions:

- vcpkg-specific things can be ignored.
- Whitespace:

  - Indents are 2 spaces.
  - There should not be spaces before parentheses in flow control and function declarations and calls.
    For example use ``if(value)``, not ``if (value)``.

- Naming:

  - Global variables and properties should be the only names in all caps.
  - Prefix public global variables with ``OPENDDS_``.
  - Prefix private global variables with ``_OPENDDS_``.
  - Prefix public functions and macros with ``opendds_``.
  - Prefix private functions and macros with ``_opendds_``.

- Prefer defining or clearing a variable before use instead of assuming that it will always be undefined.
- Don't create new macros if a function will also work.
  Functions can use ``set(name value PARENT_SCOPRE)`` to set a value in the caller's scope.
  Helper macros inside of functions are okay.
