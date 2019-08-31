#include "TimePoint_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if !defined (__ACE_INLINE__)
#  include "TimePoint_T.inl"
#endif /* __ACE_INLINE__ */
