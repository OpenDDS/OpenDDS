#include "Util.h"

#include "dds/DCPS/Definitions.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

bool bitmapNonEmpty(const OpenDDS::RTPS::SequenceNumberSet& snSet)
{
  const size_t num_ulongs = (snSet.numBits + 31) / 32;

  OPENDDS_ASSERT(num_ulongs <= snSet.bitmap.length());

  if (num_ulongs == 0) {
    return false;
  }

  const size_t last_index = num_ulongs - 1;
  for (CORBA::ULong i = 0; i < last_index; ++i) {
    if (snSet.bitmap[i]) {
      return true;
    }
  }

  const CORBA::ULong mod = snSet.numBits % 32;
  const CORBA::ULong mask = mod ? -(1u << (32 - mod)) : 0xFFFFFFFF;
  return snSet.bitmap[last_index] & mask;
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

