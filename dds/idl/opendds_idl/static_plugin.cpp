#include "be_init.h"

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

#define BACKEND_PLUGIN_FWD(NAME) \
  extern "C" { BE_Interface* NAME ## _instance(); }
#define BACKEND_PLUGIN_CHECK(NAME) \
  if (key == #NAME) { \
    return  NAME ## _instance; \
  }

// In the event that you want to add a statically linked backend into
// opendds_idl, you will need to do three things:
//   1) Use the BACKEND_PLUGIN_FWD macro to declared the backend plugin.
//   2) Use the BACKEND_PLUGIN_CHECK macro inside the static_be_plugin
//      function.
//   3) Add the backend library to the link line for opendds_idl.
//

#if defined(STATICALLY_LINK_IDL2JNI)
BACKEND_PLUGIN_FWD(idl2jni)
#endif

interface_instance_t static_be_plugin(const ACE_TString& key)
{
#if defined(STATICALLY_LINK_IDL2JNI)
  BACKEND_PLUGIN_CHECK(idl2jni)
#endif
    return 0;
}

#endif
