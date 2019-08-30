/**
 * See the "Time" section in docs/guidelines.md for background and reasoning
 * for these types.
 */

#ifndef OPENDDS_DCPS_TIME_TYPES_HEADER
#define OPENDDS_DCPS_TIME_TYPES_HEADER

#include "ace/Monotonic_Time_Policy.h"
#include "ace/Time_Policy.h"
#include "ace/Condition_Attributes.h"

#include "dds/DCPS/TimeDuration.h"
#include "dds/DCPS/TimePoint_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * ACE_Time_Policy that OpenDDS uses to define the system clock for external
 * interactions.
 */
///@{
typedef ACE_System_Time_Policy SystemClock;
typedef TimePoint_T<SystemClock> SystemTimePoint;
///@}

/**
 * ACE_Time_Policy that OpenDDS uses for internal timing.
 *
 * ACE_Monotonic_Time_Policy protects OpenDDS from being effected by changes to
 * the system clock to a certain degree. This is what will be passed from the
 * Reactor in handle_timeout.
 *
 * ConditionTime will have to be passed to ACE_Conditions to enable the
 * monotonic behavior. See the example in the "Time" section of
 * docs/guidelines.md.
 */
///@{
typedef ACE_Monotonic_Time_Policy MonotonicClock;
typedef TimePoint_T<MonotonicClock> MonotonicTimePoint;
typedef ACE_Condition_Attributes_T<MonotonicClock> ConditionTime;
///@}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
