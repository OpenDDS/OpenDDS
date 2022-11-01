#ifndef OPENDDS_IDL_WINDOWS_LINKING
#define OPENDDS_IDL_WINDOWS_LINKING

#include "opendds_idl_plugin_export.h"

#if defined(ACE_WIN32)

// An interface to set pointers to functions.  Specifically, the
// DRV_add_include_path and DRV_cpp_putarg functions provided by
// drv_preproc.cpp, which are included from $(TAO_ROOT)/TAO_IDL/driver.
opendds_idl_plugin_Export void set_drv_ptrs(void* inc, void* put);

// Define a macro that will create the function that will initialize our
// driver pointers.  This is mainly here to contain the ugliness within
// this file.
#define INITIALIZE_DRV_PTRS_FUNC \
void initialize_drv_ptrs() { \
  set_drv_ptrs(DRV_add_include_path, DRV_cpp_putarg); \
}

// Used in our drv_preproc.cpp which, in turn, calls set_drv_ptrs() with
// the actual pointers to the desired non-exported functions.
void initialize_drv_ptrs();

// The ugliest of the ugly.
// A complete hack, used inside opendds_idl.cpp, in order to get our driver
// function pointers set before anyone attempts to use them.  See
// $(TAO_ROOT)/TAO_IDL/tao_idl.cpp around line 327.
#define atc(A, B) atc(A, B); initialize_drv_ptrs()

#else
#define INITIALIZE_DRV_PTRS_FUNC
#endif

#endif /* OPENDDS_IDL_WINDOWS_LINKING */
