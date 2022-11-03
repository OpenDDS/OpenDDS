#include "be_builtin.h"

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

#define STATICALLY_LINKED_LANGUAGE_MAPPING(NAME) \
  extern "C" { LanguageMapping* opendds_idl_ ## NAME ## _allocator(); } \
  BE_BuiltinInterface::language_mapping_allocator \
  static_language_mapping(const ACE_TString& key) { \
    if (key == #NAME) { \
      return opendds_idl_ ## NAME ## _allocator; \
    } \
    return 0; \
  }

// In the event that you want to statically link the FACE language map into
// opendds_idl, you will need to do two things:
//   1) Uncomment the macro below.
//   2) Add the FACE language map library to the link line for opendds_idl.
//
//STATICALLY_LINKED_LANGUAGE_MAPPING(face);

#endif
