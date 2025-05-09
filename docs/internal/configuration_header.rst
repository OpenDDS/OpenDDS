####################
Configuration Header
####################

**********
Background
**********

At one point, MPC was the only supported means of building OpenDDS.
Configuration was done by adding entries to files like ``user_marcos.GNU``, ``default.features``, ``platform_macros.GNU``, and ``config.h``.
Later, the :ghfile:`configure` script was added to automate the generation of the necessary content.
Some of the configuration resulted in macro definitions that made various features conditional.
The macros were added to various commands like ``tao_idl``, ``opendds_idl``, and the compiler, e.g., ``-D OPENDDS_SECURITY``.

In this scheme, users could either use MPC or a custom build system for their applications.
Those using a custom build system were advised to build a sample application with MPC and then copy the necessary command-line arguments into the custom build system.
Failure to do so resulted in build failures and obscure bugs.
The root cause being that the application saw a different version of OpenDDS header files because they were processed with a different set of defined macros.

Later, CMake support was added to OpenDDS and it became necessary or least expedient to record configuration that affected preprocessing in a conventional header file ``dds/OpenDDSConfig.h``.
The :ghfile:`configure` script was capable of generating the same header file.

The configuration header solves the problem where applications using OpenDDS are built using a different set of definitions than were used to compile OpenDDS itself.
A purely cosmetic benefit is that macros no longer need to be defined on the command line.

******
Design
******

The configuration header is named ``dds/OpenDDSConfig.h``.

The configuration header is mandatory.
At the time of design ``__has_include`` was not supported by the majority of compilers.
The work around was for OpenDDS to provide a default and manipulate include paths to prefer a user provided file.
The approach was deemed unmaintainable.

The macros in ``dds/OpenDDSConfig.h`` are named ``OPENDDS_CONFIG_X``.
This name was chosen to not clash for backwards compatibility.

The macros in ``dds/OpenDDSConfig.h`` are defined to 0, 1, or another value.
This allows the code to use preprocessor logic while enabling the use of ``-Wundef``.
A macro that is undefined can be set to a default value.

``dds/OpenDDSConfig.h`` only contain macros and comments.
This allows it to be included from C++ and IDL.

``dds/OpenDDSConfig.h`` is accessed through a wrapper called :ghfile:`dds/OpenDDSConfigWrapper.h`.
The wrapper has three functions:

1. Set a default value for undefined macros.
2. Ensure consistency between legacy macros and the current set of macros.
3. Perform sanity checks on the chosen configuration.

Various versions of ``dds/OpenDDSConfigWrapper.X`` exist so that :ghfile:`dds/OpenDDSConfigWrapper.h` can be included in IDL and code generated from IDL.

:ghfile:`dds/OpenDDSConfig.h.in` is a template used by :ghfile:`configure` and CMake.
It can also be used by users that want to manually create ``dds/OpenDDSConfig.h``.

******************
Adding a New Macro
******************

1. Add the macro to :ghfile:`dds/OpenDDSConfig.h.in`.
2. Add support in :ghfile:`configure` and CMake.
3. Add a default and any other necessary support in :ghfile:`dds/OpenDDSConfigWrapper.h`.
4. Update :ref:`building--configuration-header` to document the new macro.

************
Using Macros
************

C++ code should include :ghfile:`dds/OpenDDSConfigWrapper.h` or :ghfile:`dds/DCPS/Definitions.h`.
IDL should include :ghfile:`dds/OpenDDSConfigWrapper.idl`.
Generated code will use a ``dds/OpenDDSConfigWrapper.X`` variant.

The included header must appear before the first usage of a macro.

Do not check if the macro is defined; assume it is.
If the macro is not defined, then there will usually be a build error.
Similarly, ``-Wundef`` can be used to find undefined macros.

If both a source file and a header file require the configuration header, then the configuration header should be included in the header and not in the source file.
This leads to the following pattern when entire files are conditionally defined:

.. code-block:: cpp

    #ifndef SOME_HEADER_H
    #define SOME_HEADER_H

    #include <dds/DCPS/Definitions.h>

    #if OPENDDS_CONFIG_X
    ...
    #endif

    #endif // SOME_HEADER_H

.. code-block:: cpp

   #include "SomeHeader.h"

   #if OPENDDS_CONFIG_X
    ...
   #endif
