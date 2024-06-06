/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TIME_HELPER_H
#define OPENDDS_DCPS_TIME_HELPER_H

#include "PoolAllocator.h"
#include "dcps_export.h"

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/Monotonic_Time_Policy.h>
#include <ace/OS_NS_sys_time.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename TimeT>
ACE_Time_Value time_to_time_value(const TimeT& t)
{
  ACE_Time_Value tv(t.sec, t.nanosec / 1000);
  return tv;
}

template <typename IdlType>
void time_value_to_time(IdlType& t, const ACE_Time_Value& tv)
{
  t.sec = ACE_Utils::truncate_cast<CORBA::Long>(tv.sec());
  t.nanosec = ACE_Utils::truncate_cast<CORBA::ULong>(tv.usec() * 1000);
}

ACE_INLINE OpenDDS_Dcps_Export
DDS::Time_t time_value_to_time(const ACE_Time_Value& tv);

ACE_INLINE OpenDDS_Dcps_Export
MonotonicTime_t time_value_to_time(const ACE_Time_Value_T<ACE_Monotonic_Time_Policy>& tv);

ACE_INLINE OpenDDS_Dcps_Export
ACE_Time_Value duration_to_time_value(const DDS::Duration_t& t);

ACE_INLINE OpenDDS_Dcps_Export
ACE_Time_Value duration_to_absolute_time_value(const DDS::Duration_t& t,
                                               const ACE_Time_Value& now);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t time_value_to_duration(const ACE_Time_Value& tv);

ACE_INLINE OpenDDS_Dcps_Export
double time_value_to_double(const ACE_Time_Value& tv);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t time_to_duration(const DDS::Time_t& t);

/// Validate DDS::Duration_t value (infinite or positive and
/// non-zero).
ACE_INLINE OpenDDS_Dcps_Export
bool valid_duration(DDS::Duration_t const & t);

/// Check if given duration is either infinite or greater than or
/// equal to zero.
ACE_INLINE OpenDDS_Dcps_Export
bool non_negative_duration(const DDS::Duration_t& t);

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::Duration_t& t1, const DDS::Duration_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::Duration_t& t1, const DDS::Duration_t& t2);
#endif

ACE_INLINE OpenDDS_Dcps_Export
bool operator<(const DDS::Duration_t& t1, const DDS::Duration_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator<=(const DDS::Duration_t& t1, const DDS::Duration_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator>(const DDS::Duration_t& t1, const DDS::Duration_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator>=(const DDS::Duration_t& t1, const DDS::Duration_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!(const DDS::Time_t& t);

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator!=(const DDS::Time_t& t1, const DDS::Time_t& t2);
#endif

ACE_INLINE OpenDDS_Dcps_Export
bool operator<(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator<=(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator>(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator>=(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Time_t operator+(const DDS::Time_t& t1, const DDS::Duration_t& d1);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t operator-(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Time_t operator-(const DDS::Time_t& t1, const DDS::Duration_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t operator-(const MonotonicTime_t& t1, const MonotonicTime_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
bool operator<(const MonotonicTime_t& t1, const MonotonicTime_t& t2);

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE OpenDDS_Dcps_Export
bool operator==(const MonotonicTime_t& t1, const MonotonicTime_t& t2);
#endif

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 uint32_fractional_seconds_to_nanoseconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 nanoseconds_to_uint32_fractional_seconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 uint32_fractional_seconds_to_microseconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 microseconds_to_uint32_fractional_seconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
bool is_infinite(const DDS::Duration_t& value);

ACE_INLINE OpenDDS_Dcps_Export
const MonotonicTime_t& monotonic_time_zero();

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t make_duration_t(int sec, unsigned long nanosec);

ACE_INLINE OpenDDS_Dcps_Export
String to_dds_string(const DDS::Duration_t& duration);

ACE_INLINE OpenDDS_Dcps_Export
bool from_dds_string(const String& str, DDS::Duration_t& duration);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Time_t make_time_t(int sec, unsigned long nanosec);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "Time_Helper.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TIME_HELPER_H */
