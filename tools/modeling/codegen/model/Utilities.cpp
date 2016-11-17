
#include "Utilities.h"
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

void
OpenDDS::Model::stringToByteSeq(
  DDS::OctetSeq& target,
  const char*    source)
{
  const size_t len = std::strlen( source);
  target.length(static_cast<CORBA::ULong>(len));

  if( len) {
    std::memcpy( target.get_buffer(), source, len);
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
