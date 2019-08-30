#include "TimePoint_T.h"

namespace OpenDDS {
namespace DCPS {

template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::zero_value = TimePoint_T<AceClock>(ACE_Time_Value::zero);

template<typename AceClock>
const TimePoint_T<AceClock> TimePoint_T<AceClock>::max_value = TimePoint_T<AceClock>(ACE_Time_Value::max_time);

template<typename AceClock>
AceClock TimePoint_T<AceClock>::clock;

}
}

#if !defined (__ACE_INLINE__)
#  include "TimePoint_T.inl"
#endif /* __ACE_INLINE__ */
