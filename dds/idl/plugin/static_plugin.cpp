#include "be_init.h"

// In order to statically link backends into the opendds_idl executable, we
// provide a function to allocate and return backend interfaces that are known
// at link time.
//
// This file (and function) is a place holder so that it can be replaced
// by a version provided by the opendds_idl executable.


#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

interface_instance_t static_be_plugin(const ACE_TString& key)
{
  return 0;
}

#endif
