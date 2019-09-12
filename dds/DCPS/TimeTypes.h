/**
 * Monotonic Time Backport for 3.13. Just provides the bare minimum needed for
 * monotonic time.
 */

#ifndef OPENDDS_DCPS_TIME_TYPES_HEADER
#define OPENDDS_DCPS_TIME_TYPES_HEADER

#include "ace/Monotonic_Time_Policy.h"
#include "ace/Condition_Attributes.h"
#include "ace/Time_Policy.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * ACE_Time_Policy that OpenDDS uses for internal timing.
 *
 * ACE_Monotonic_Time_Policy protects OpenDDS from being affected by changes to
 * the system clock to a certain degree.
 */
///@{
#if defined(__APPLE__) && defined(__MACH__)
/*
 * As of writing, ACE_Monotonic_Time_Policy doesn't support Darwin systems like
 * macOS. Use ACE_System_Time_Policy instead, because ACE_Monotonic_Time_Policy
 * falls back to returning ACE_Time_Value::zero for some reason.
 */
typedef ACE_System_Time_Policy MonotonicClock;
#else
typedef ACE_Monotonic_Time_Policy MonotonicClock;
#endif
inline
ACE_Time_Value_T<MonotonicClock>
monotonic_time()
{
  static MonotonicClock clock;
  return clock();
}
///@}

/**
 * ConditionAttributesMonotonic will have to be passed to ACE_Condition for
 * it to interpret the ACE_Time_Value* argument of wait as monotonic time.
 */
typedef ACE_Condition_Attributes_T<MonotonicClock> ConditionAttributesMonotonic;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
