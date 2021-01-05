#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TimeDuration.h"
#ifdef ACE_HAS_CPP11
#  include <limits>
#endif /* ACE_HAS_CPP11*/

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const TimeDuration TimeDuration::zero_value(0, 0);
#ifdef ACE_HAS_CPP11
const TimeDuration TimeDuration::max_value(std::numeric_limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1);
#else
const TimeDuration TimeDuration::max_value(ACE_Numeric_Limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1);
#endif /* ACE_HAS_CPP11 */

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if !defined (__ACE_INLINE__)
#  include "TimeDuration.inl"
#endif /* __ACE_INLINE__ */
