/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TIME_HELPER_H
#define OPENDDS_DCPS_TIME_HELPER_H

#include "dds/DdsDcpsCoreC.h"
#include "ace/OS_NS_sys_time.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE OpenDDS_Dcps_Export
ACE_Time_Value time_to_time_value(const DDS::Time_t& t);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Time_t time_value_to_time(const ACE_Time_Value& tv);

ACE_INLINE OpenDDS_Dcps_Export
ACE_Time_Value duration_to_time_value(const DDS::Duration_t& t);

ACE_INLINE OpenDDS_Dcps_Export
ACE_Time_Value duration_to_absolute_time_value(const DDS::Duration_t& t,
                                               const ACE_Time_Value& now);

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t time_value_to_duration(const ACE_Time_Value& tv);

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
DDS::Time_t operator-(const DDS::Time_t& t1, const DDS::Time_t& t2);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 uint32_fractional_seconds_to_nanoseconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 nanoseconds_to_uint32_fractional_seconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 uint32_fractional_seconds_to_microseconds(ACE_UINT32 fraction);

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 microseconds_to_uint32_fractional_seconds(ACE_UINT32 fraction);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "Time_Helper.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TIME_HELPER_H */
