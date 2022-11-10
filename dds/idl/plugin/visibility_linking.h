/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

// The purpose of this file is to provide link level visibility for the
// DRV_add_include_path() and DRV_cpp_putarg() functions that are defined
// in $TAO_ROOT/TAO_IDL/driver/drv_preproc.cpp.  Those functions are
// referenced by opendds_idl_plugin framework library, but are not defined
// within it.  The opendds_idl executable project includes the afore mentioned
// file into it via a #include.  Because these functions (and others defined
// in the .cpp) are not exported, we have to either employ this work-around
// or reproduce the content of drv_preproc.{h,cpp} with exports.

#ifndef OPENDDS_IDL_VISIBILITY_LINKING
#define OPENDDS_IDL_VISIBILITY_LINKING

#include "opendds_idl_plugin_export.h"

#if defined(ACE_Proper_Export_Flag) && \
    defined (OPENDDS_IDL_PLUGIN_HAS_DLL) && (OPENDDS_IDL_PLUGIN_HAS_DLL == 1)
#include <ace/SString.h>

typedef ACE_CString&
       (*add_include_path)(ACE_CString& include_path, const char* path,
                           const char* suffix, bool is_system);
typedef void (*cpp_putarg)(const char* str);

// An interface to set pointers to functions.  Specifically, the
// DRV_add_include_path and DRV_cpp_putarg functions provided by
// drv_preproc.cpp, which are included from $(TAO_ROOT)/TAO_IDL/driver.
opendds_idl_plugin_Export void set_drv_ptrs(add_include_path inc_func,
                                            cpp_putarg put_func);

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

#endif /* OPENDDS_IDL_VISIBILITY_LINKING */
