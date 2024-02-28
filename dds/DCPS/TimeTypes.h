/**
 * \file
 * See https://opendds.readthedocs.io/en/master/internal/dev_guidelines.html#time
 * (or docs/internal/dev_guidelines.rst) for background and reasoning for these
 * types.
 */

#ifndef OPENDDS_DCPS_TIMETYPES_H
#define OPENDDS_DCPS_TIMETYPES_H

#include "Definitions.h"
#include "TimeDuration.h"
#include "TimePoint_T.h"
#include "dcps_export.h"

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
 * MonotonicClock protects OpenDDS from being affected by changes to
 * the system clock to a certain degree.
 *
 * CLOCK_BOOTTIME is a monotonic clock that includes the time the system is suspended.
 * If OPENDDS_CONFIG_MONOTONIC_USES_BOOTTIME is true, CLOCK_BOOTTIME will be
 * used in place of CLOCK_MONOTONIC on platforms that support it.  The types
 * used by OpenDDS are still named MonotonicClock and MonotonicTimePoint.
 */
///@{
#if defined(ACE_HAS_MONOTONIC_TIME_POLICY) && defined(ACE_HAS_MONOTONIC_CONDITIONS)
#  define OPENDDS_USES_MONOTONIC_TIME
#endif

#if OPENDDS_CONFIG_MONOTONIC_USES_BOOTTIME

struct OpenDDS_Dcps_Export BootTimePolicy {
  ACE_Time_Value_T<BootTimePolicy> operator()() const;
  void set_gettimeofday(ACE_Time_Value (*)()) {} // see comment in ace/Monotonic_Time_Policy.h
};

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
ACE_BEGIN_VERSIONED_NAMESPACE_DECL

template <>
struct OpenDDS_Dcps_Export ACE_Condition_Attributes_T<OpenDDS::DCPS::BootTimePolicy>
  : ACE_Condition_Attributes {
  ACE_Condition_Attributes_T(int type = ACE_DEFAULT_SYNCH_TYPE);
};

ACE_END_VERSIONED_NAMESPACE_DECL
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

typedef BootTimePolicy MonotonicClock;
#elif defined OPENDDS_USES_MONOTONIC_TIME
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
