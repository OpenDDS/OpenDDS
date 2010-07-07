
#include "Utilities.h"
#include <cstring>

void
OpenDDS::Model::stringToByteSeq(
  DDS::OctetSeq& target,
  const char*    source
)
{
  size_t len = std::strlen( source);
  if( len == 0) {
    target.length( 0);

  } else {
    target.length( len);
    std::memcpy( target.get_buffer(), source, len);
  }
}

