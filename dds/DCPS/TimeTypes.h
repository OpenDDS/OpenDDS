/**
 * \file
 * See https://opendds.readthedocs.io/en/master/internal/dev_guidelines.html#time
 * (or docs/internal/dev_guidelines.rst) for background and reasoning for these
 * types.
 */

#ifndef OPENDDS_DCPS_TIMETYPES_H
#define OPENDDS_DCPS_TIMETYPES_H

#include "TimeDuration.h"
#include "TimePoint_T.h"

#include <ace/Monotonic_Time_Policy.h>
#include <ace/Time_Policy.h>

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

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
