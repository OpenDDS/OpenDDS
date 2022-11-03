#include "be_init.h"

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

interface_instance_t static_be_plugin(const ACE_TString& key)
{
  return 0;
}

#endif
