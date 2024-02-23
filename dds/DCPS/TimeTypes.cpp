#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "TimeTypes.h"

#include <ace/os_include/os_time.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

#if OPENDDS_CONFIG_MONOTONIC_USES_BOOTTIME

ACE_Time_Value_T<BootTimePolicy> BootTimePolicy::operator()() const
{
  timespec ts;
  if (0 == ::clock_gettime(CLOCK_BOOTTIME, &ts)) {
    return ACE_Time_Value_T<BootTimePolicy>(ts);
  }

  return ACE_Time_Value_T<BootTimePolicy>(ACE_Time_Value::zero);
}

#endif

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
