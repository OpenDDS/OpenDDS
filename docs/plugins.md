# OpenDDS IDL Compiler Plugins

The OpenDDS IDL compiler supports two different type of plugins.  It has the ability to load multiple backend plugins, which would be responsible for generating the resultant artifacts after processing IDL files.  It uses the default built-in backend if the user does not provide an alternate backend.  Additionally, the built-in backend supports dynamically loading a language mapping plugin; only one language mapping can be active at any given time.

**Table of Contents:**

* [Usage](#usage)
* [Creating Backend Plugins](#creating-backend-plugins)
* [Adding Language Mapping Plugins](#adding-language-mapping-plugins)
* [Static Linking](#static-linking)
* [TAO_IDL ugliness](#tao_idl-ugliness)

## Usage

Provide the `--plugin` option on the `opendds_idl` command line to load a specific backend.  The argument to the `--plugin` option is the library name (without prefix or suffix) that contains the backend code.  Each loaded backend is given the opportunity to process command line options, once it is loaded.

It is currently possible to load multiple backends.  However, there is no mechanism to direct specific command line options supported by the backends to the specific backend for which the option is destined.  As it currently stands, each backend would need to support the same command line options or ignore options that it does not recognize.

The built-in backend has the ability to dynamically load language mapping plugins.  In order to load a language mapping plugin, the user would provide the `-L` option on the `opendds_idl` command line.  The text that directly follows the `-L` (no space after) is used as to create the name of the language mapping plugin library.  For example, `-Lface` would tell `opendds_idl` to look for a library named `opendds_idl_face`.  In addition to the `-L` option, the `-G...TS` option can be used to indicate that Type Support generation should be enabled within the language mapping, e.g., `-GfaceTS`.  A user-implemented language mapping cannot be named `spcpp` or `c++11`, as those are part of the built-in backend that are always available.

## Creating Backend Plugins

Creating a backend plugin requires a concrete implementation of the abstract base class `BE_Interface` and a C linkage function that provides a pointer to an instance of the concrete implementation.  The `BE_Interface` class is defined in `$DDS_ROOT/dds/idl/plugin/be_interface.h`.  Each pure virtual method in the class is documented as to it's use and general responsibility.  The C linkage function must be named such that it starts with the library name and is followed by `_instance`.  It need not allocate the concrete implementation via `new`, but does need to provide a pointer to it.  For example:

```
extern "C" {
  libname_Export BE_Interface* libname_instance()
  {
    static BE_LIBNAMEInterface instance;
    return &instance;
  }
}
```

To see concrete examples of implementations of the `BE_Interface` class, see `$DDS_ROOT/dds/idl/plugin/be_builtin.{h,cpp}` and `$DDS_ROOT/java/idl2jni/codegen/be_jni.{h,cpp}`.

## Adding Language Mapping Plugins

The built-in backend supports code generation using a single active language mapping.  In order to add a language mapping to be dynamically loaded by the the built-in backend, you must implement an instance of the `LanguageMapping` class that overrides one or more virtual functions.  Most of the code that actually generates the code for the backend language mapping is spread across multiple C++ files found in `$DDS_ROOT/dds/idl/plugin`.  Therefore, the language mapping is mostly a way to direct the backend code to generate code in a specific way, with a bit of code generation as well.

The `LanguageMapping` class is defined in `$DDS_ROOT/dds/idl/plugin/language_mapping.h` and each virtual function is documented as to it's responsibilities.  See `$DDS_ROOT/dds/idl/face/face_language_mapping.{h,cpp}` for an example of an implementation of the `LanguageMapping`.

## Static Linking

It is possible to statically link the existing plugins and language mappings into `opendds_idl`.  However, as it is currently implemented, this requires manual intervention.  When building `opendds_idl`, you must define a macro relating to the plugin or language mapping and add the related library to the link line.

To statically link the `FACE` language mapping into `opendds_idl`, you would need to add the following to the project in `$DDS_ROOT/dds/idl/opendds_idl/opendds_idl.mpc`:

```
  macros += STATICALLY_LINK_FACE
  after  += opendds_idl_face
  libs   += opendds_idl_face
```

To statically link the `IDL2JNI` backend into `opendds_idl`, you would need to add the following to the project in `$DDS_ROOT/dds/idl/opendds_idl/opendds_idl.mpc`:
```
  macros += STATICALLY_LINK_IDL2JNI
  after  += idl2jni_codegen
  libs   += idl2jni
```
As well as, enabling the `java` feature in `MPC`.

This is cumbersome and is expected to change in the future as we progress within v4 of OpenDDS.

## TAO_IDL Ugliness

As it is currently written, the OpenDDS IDL compiler includes a portion of the `tao_idl` compiler code to provide the `main` function for the executable along with other functionality relating to the command line interface.  Because some of the IDL compiler functionality has been split out of the monolithic executable into separate libraries, some of the functions that are included directly into the executable need to be exported, on platforms that support function visibility, so that they can be accessed within the libraries.

Since the future version of `opendds_idl` will not be based on `tao_idl`, it was chosen to use a temporary work-around to the lack of visibility of required functions.  See `$DDS_ROOT/dds/idl/plugin/visibility_linking.{h,cpp}` for the ugly details.  This should all be removed once `opendds_idl` is re-implemented to remove TAO-specific code.
