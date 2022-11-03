#include "be_builtin.h"

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

BE_BuiltinInterface::language_mapping_allocator
static_language_mapping(const ACE_TString& key)
{
  return 0;
}

#endif
