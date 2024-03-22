#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "TimeTypes.h"

#if OPENDDS_CONFIG_MONOTONIC_USES_BOOTTIME

#include <ace/os_include/os_time.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

ACE_Time_Value_T<BootTimePolicy> BootTimePolicy::operator()() const
{
  timespec ts;
  if (0 == ::clock_gettime(CLOCK_BOOTTIME, &ts)) {
    return ACE_Time_Value_T<BootTimePolicy>(ts);
  }

  return ACE_Time_Value_T<BootTimePolicy>(ACE_Time_Value::zero);
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_Condition_Attributes_T<OpenDDS::DCPS::BootTimePolicy>::ACE_Condition_Attributes_T(int type)
 : ACE_Condition_Attributes(type)
{
  (void) ACE_OS::condattr_setclock(attributes_, CLOCK_BOOTTIME);
}

ACE_END_VERSIONED_NAMESPACE_DECL

#endif
