#include "TimePoint_T.h"

#ifdef ACE_HAS_CPP11
#  include <limits>
#else
#  include <ace/Numeric_Limits.h>
#endif /* ACE_HAS_CPP11*/

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::zero_value(ACE_Time_Value(0, 0));

#ifdef ACE_HAS_CPP11
template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::max_value(ACE_Time_Value(std::numeric_limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1));
#else
template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::max_value(ACE_Time_Value(ACE_Numeric_Limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1));
#endif /* ACE_HAS_CPP11 */

template<typename AceClock>
AceClock TimePoint_T<AceClock>::clock;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if !defined (__ACE_INLINE__)
#  include "TimePoint_T.inl"
#endif /* __ACE_INLINE__ */
