
#include "Utilities.h"
#include <cstring>

void
OpenDDS::Model::stringToByteSeq(
  DDS::OctetSeq& target,
  const char*    source
)
{
  size_t len = std::strlen( source);
  target.length( len);

  if( len) {
    std::memcpy( target.get_buffer(), source, len);
  }
}

