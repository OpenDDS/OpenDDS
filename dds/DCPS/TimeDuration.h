#ifndef OPENDDS_DCPS_TIME_DURATION_HEADER
#define OPENDDS_DCPS_TIME_DURATION_HEADER

#include <ace/Time_Value.h>

#include "dds/DdsDcpsCoreC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Represents a length of time and based on C++11 std::chrono::duration.
 *
 * This wraps an ACE_Time_Value, and is designed to work with TimePoint_T.
 *
 * See the "Time" section in docs/guidelines.md for background and reasoning
 * for this class.
 */
class OpenDDS_Dcps_Export TimeDuration {
public:
  const static TimeDuration zero_value;
  const static TimeDuration max_value;

  /**
   * Return a TimeDuration equivalent to the given number of milliseconds.
   */
  static TimeDuration from_msec(const ACE_UINT64& ms);

  /**
   * Initialized to zero
   */
  TimeDuration();

  TimeDuration(const TimeDuration& other);

  /**
   * Copy the ACE_Time_Value directly.
   *
   * \warning It will accept a ACE_Time_Value that represents a specific point
   * in time, but it is wrong to do so. Use one of the TimePoint_T classes
   * defined in TimeTypes.h that match the clock type.
   */
  explicit TimeDuration(const ACE_Time_Value& ace_time_value);

  /**
   * Define the number of seconds and optionally microseconds, passed to
   * ACE_Time_Value's constructor of the same signature.
   */
  explicit TimeDuration(time_t sec, suseconds_t usec = 0);

  /**
   * Converts the DDS Duration into the equivalent value.
   */
  explicit TimeDuration(const DDS::Duration_t& dds_duration);

  const ACE_Time_Value& value() const;
  void value(const ACE_Time_Value& ace_time_value);
  bool is_zero() const;
  bool is_max() const;
  DDS::Duration_t to_dds_duration() const;

  TimeDuration& operator+=(const TimeDuration& other);
  TimeDuration& operator-=(const TimeDuration& other);
  TimeDuration& operator=(const TimeDuration& other);
  TimeDuration& operator=(const time_t& other);
  TimeDuration& operator*=(double other);

private:
  ACE_Time_Value value_;
};

OpenDDS_Dcps_Export TimeDuration operator+(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export TimeDuration operator-(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export TimeDuration operator-(const TimeDuration& x);
OpenDDS_Dcps_Export TimeDuration operator*(double x, const TimeDuration& y);
OpenDDS_Dcps_Export TimeDuration operator*(const TimeDuration& x, double y);
OpenDDS_Dcps_Export bool operator<(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator>(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator<=(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator>=(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator==(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator!=(const TimeDuration& x, const TimeDuration& y);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#  include "TimeDuration.inl"
#endif /* __ACE_INLINE__ */

#endif
