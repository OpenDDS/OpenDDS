#include "be_init.h"

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

#define STATICALLY_LINKED_BACKEND_PLUGIN(NAME) \
  extern "C" { BE_Interface* NAME ## _instance(); } \
  interface_instance_t static_be_plugin(const ACE_TString& key) { \
    if (key == #NAME) { \
      return  NAME ## _instance; \
    } \
    return 0; \
  }

// In the event that you want to statically link the idl2jni backend into
// opendds_idl, you will need to do two things:
//   1) Uncomment the macro below.
//   2) Add the idl2jni backend library to the link line for opendds_idl.
//
//STATICALLY_LINKED_BACKEND_PLUGIN(idl2jni);

#endif
