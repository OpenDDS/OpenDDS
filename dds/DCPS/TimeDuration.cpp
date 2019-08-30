#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TimeDuration.h"

namespace OpenDDS {
namespace DCPS {

const TimeDuration TimeDuration::zero_value = TimeDuration(ACE_Time_Value::zero);
const TimeDuration TimeDuration::max_value = TimeDuration(ACE_Time_Value::max_time);

}
}

#if !defined (__ACE_INLINE__)
#  include "TimeDuration.inl"
#endif /* __ACE_INLINE__ */
