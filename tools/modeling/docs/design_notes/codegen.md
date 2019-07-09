<!--
  This file was converted manually from codegen.html TiddlyWiki.
  That had several pages in it's table of contents, of which only two were not
  empty and were copied here.
-->
# OpenDDS Modeling SDK Code Generation
This document describes the design and rationale for the code generation
portion of the OpenDDS Modeling SDK. This includes the content and format of
the XML model files as well as the translation steps and the resulting output
files created. Currently a C++ header and implementation (`.h` and `.cpp`)
file, an IDL data description (`.idl`) file, an MPC project description
(`.mpc`) file and an export definition (`_export.h`) C++ header file are
created for each model that is being generated.

Application code wanting to use models that have been generated have several
different options on how to access the model defined middleware.

# Generated Code

The generated output code resides in various files. These include the following formats and content:

| File | Example | Description |
| ---- | ------- | ----------- |
| C++ export file | `modelname_export.h` | This is a header file defining the export macros used by the C++ code when creating the model library. |
| C++ Header file | `modelname.h` | This is the header file defining the enumerations and structures holding the data from the model definition. |
| C++ Implementation file | `modelname.cpp` | This is the C++ body where the data from the model is made available to the C++ code. |
| IDL file | `modelname.idl` | This contains the IDL data definitions for the data used by the model middleware. |
| MPC project file | `modelname.mpc` | This is the project definition file used by MPC to create the required build artifacts for the chosen toolchain. |

These generated files rely on existing modeling support from OpenDDS as well.
This support is provided in the form of headers and libraries available for
building and linking with the model file. These support files are located in
the `$DDS_ROOT/modeling/tools/codegen/model` directory and build the library
`libOpenDDS_Model.so` located in the `$DDS_ROOT/lib` directory. The support
files include:

| File | Description |
| ---- | ----------- |
| `Service_T.{h,cpp}` | These define a template class which can be specialized using the generated model Elements class to create an instantiable model. It provides interfaces for creating and initializing each element defined in the model. |
| `Delegate.{h,cpp}` | These provide implementations for the interfaces defined in the `Service_T` template files. The operations from the template class are delegated to this class for execution. |
| `Exceptions.h` | This header defines many individual `std::exception` implementations which are thrown at various points in both the support code and the generated model code. |
| `libOpenDDS_Model.so` | This library contains the linkable version of the support files. |

The generated code and support code includes the following elements:

 - Service class interfaces
 - Delegate class implementations
 - model IDL definitions
 - model Elements class interfaces
 - model enumerations and values
 - model Data class and implementations
 - Exception classes
