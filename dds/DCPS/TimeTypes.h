/**
 * \file
 * See the "Time" section in docs/guidelines.md for background and reasoning
 * for these types.
 */

#ifndef OPENDDS_DCPS_TIME_TYPES_HEADER
#define OPENDDS_DCPS_TIME_TYPES_HEADER

#include "TimeDuration.h"
#include "TimePoint_T.h"

#include <ace/Monotonic_Time_Policy.h>
#include <ace/Time_Policy.h>
#include <ace/Condition_Attributes.h>

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
 * the system clock to a certain degree.
 */
///@{
#if defined(ACE_HAS_MONOTONIC_TIME_POLICY) && defined(ACE_HAS_MONOTONIC_CONDITIONS)
#  define OPENDDS_USES_MONOTONIC_TIME
#endif

#ifdef OPENDDS_USES_MONOTONIC_TIME
typedef ACE_Monotonic_Time_Policy MonotonicClock;
#else
typedef SystemClock MonotonicClock;
#endif
typedef TimePoint_T<MonotonicClock> MonotonicTimePoint;
///@}

/**
 * ConditionAttributesMonotonic() will have to be passed as the second argument
 * in an ACE_Condition constructor for it to interpret argument of
 * wait(ACE_Time_Value*) as monotonic time.
 */
typedef ACE_Condition_Attributes_T<MonotonicClock> ConditionAttributesMonotonic;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
