#include "Util.h"

/// Return the number of CORBA::Longs required for the bitmap representation of
/// sequence numbers between low and high, inclusive (maximum 8 longs).
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

CORBA::ULong
bitmap_num_longs(const OpenDDS::DCPS::SequenceNumber& low,
                 const OpenDDS::DCPS::SequenceNumber& high)
{
  return high < low ? CORBA::ULong(0) : std::min(CORBA::ULong(8), CORBA::ULong((high.getValue() - low.getValue() + 32) / 32));
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

