#include "Util.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

CORBA::ULong
bitmap_num_longs(const SequenceNumber& low, const SequenceNumber& high)
{
  return high < low ? CORBA::ULong(0) : std::min(CORBA::ULong(8), CORBA::ULong((high.getValue() - low.getValue() + 32) / 32));
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

