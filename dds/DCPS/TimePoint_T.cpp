#include "TimePoint_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::zero_value(ACE_Time_Value(0, 0));

template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::max_value(ACE_Time_Value(ACE_Numeric_Limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1));

template<typename AceClock>
AceClock TimePoint_T<AceClock>::clock;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if !defined (__ACE_INLINE__)
#  include "TimePoint_T.inl"
#endif /* __ACE_INLINE__ */
