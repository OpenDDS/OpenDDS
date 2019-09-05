/**
 * Monotonic Time Backport for 3.14. Just provides the bare minimum needed for
 * monotonic time.
 */

#ifndef OPENDDS_DCPS_TIME_TYPES_HEADER
#define OPENDDS_DCPS_TIME_TYPES_HEADER

#include "ace/Monotonic_Time_Policy.h"
#include "ace/Condition_Attributes.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * ACE_Time_Policy that OpenDDS uses for internal timing.
 *
 * ACE_Monotonic_Time_Policy protects OpenDDS from being effected by changes to
 * the system clock to a certain degree. This is what will be passed from the
 * Reactor in handle_timeout.
 *
 * ConditionTime will have to be passed to ACE_Conditions to enable the
 * monotonic behavior.
 */
///@{
typedef ACE_Monotonic_Time_Policy MonotonicClock;
inline
ACE_Time_Value_T<MonotonicClock>
monotonic_time()
{
  static MonotonicClock clock;
  return clock();
}
typedef ACE_Condition_Attributes_T<MonotonicClock> ConditionTime;
///@}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
