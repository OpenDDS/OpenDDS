#include "be_builtin.h"

// In order to statically link language mappings into the opendds_idl
// executable, we provide a function to allocate and return language mappings
// that are known at link time.
//
// This file (and function) is a place holder so that it can be replaced
// by a version provided by the opendds_idl executable.

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

BE_BuiltinInterface::language_mapping_allocator
static_language_mapping(const ACE_TString& key)
{
  return 0;
}

#endif
