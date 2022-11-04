#include "be_builtin.h"

#if defined (ACE_AS_STATIC_LIBS)
#include <ace/SString.h>

#define LANGUAGE_MAPPING_FWD(NAME) \
  extern "C" { LanguageMapping* opendds_idl_ ## NAME ## _allocator(); }
#define LANGUAGE_MAPPING_CHECK(NAME) \
  if (key == #NAME) { \
    return opendds_idl_ ## NAME ## _allocator; \
  }

// In the event that you want to add a statically linked the language map into
// opendds_idl, you will need to do three things:
//   1) Use the LANGUAGE_MAPPING_FWD macro to declare the language map.
//   2) Use the LANGUAGE_MAPPING_CHECK macro inside the
//      static_language_mapping function.
//   3) Add the language map library to the link line for opendds_idl.
//

#if defined(STATICALLY_LINK_FACE)
LANGUAGE_MAPPING_FWD(face)
#endif

BE_BuiltinInterface::language_mapping_allocator
static_language_mapping(const ACE_TString& key) {
#if defined(STATICALLY_LINK_FACE)
  LANGUAGE_MAPPING_CHECK(face)
#endif
  return 0;
}

#endif
