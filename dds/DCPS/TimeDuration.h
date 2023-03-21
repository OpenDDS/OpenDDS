#ifndef OPENDDS_DCPS_TIMEDURATION_H
#define OPENDDS_DCPS_TIMEDURATION_H

#include "PoolAllocator.h"
#include "SafeBool_T.h"

#include <dds/DdsDcpsCoreC.h>

#include <ace/Time_Value.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Represents a length of time and based on C++11 std::chrono::duration.
 *
 * This wraps an ACE_Time_Value, and is designed to work with TimePoint_T.
 *
 * See https://opendds.readthedocs.io/en/master/internal/dev_guidelines.html#time
 * (or docs/internal/dev_guidelines.rst) for background and reasoning for this
 * class.
 */
class OpenDDS_Dcps_Export TimeDuration : public SafeBool_T<TimeDuration> {
public:
  const static TimeDuration zero_value;
  const static TimeDuration max_value;

  /**
   * Return a TimeDuration equivalent to the given number of milliseconds.
   */
  static TimeDuration from_msec(const ACE_UINT64& ms);

  /**
   * Return a TimeDuration from fractional seconds.
   */
  static TimeDuration from_double(double duration);

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

  bool boolean_test() const
  {
    return value_ != zero_value.value();
  }

  /**
   * Convert to a string in a humanized format:
   *    SECONDS.FRACTIONAL s
   * if the time is less than a minute or just_sec is true, else:
   *    [[HOURS:]MINUTES:]SECONDS.FRACTIONAL
   *
   * decimal_places is the number of decimal places to round to in the
   * fractional seconds.
   */
  String str(unsigned decimal_places = 3, bool just_sec = false) const;

  String sec_str(unsigned decimal_places = 3) const
  {
    return str(decimal_places, true);
  }

  TimeDuration& operator+=(const TimeDuration& other);
  TimeDuration& operator-=(const TimeDuration& other);
  TimeDuration& operator=(const TimeDuration& other);
  TimeDuration& operator=(const time_t& other);
  TimeDuration& operator*=(double other);
  TimeDuration& operator/=(double other);

protected:
  ACE_Time_Value value_;
};

OpenDDS_Dcps_Export TimeDuration operator+(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export TimeDuration operator-(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export TimeDuration operator-(const TimeDuration& x);
OpenDDS_Dcps_Export TimeDuration operator*(double x, const TimeDuration& y);
OpenDDS_Dcps_Export TimeDuration operator*(const TimeDuration& x, double y);
OpenDDS_Dcps_Export TimeDuration operator/(const TimeDuration& x, double y);
OpenDDS_Dcps_Export double operator/(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator<(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator>(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator<=(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator>=(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator==(const TimeDuration& x, const TimeDuration& y);
OpenDDS_Dcps_Export bool operator!=(const TimeDuration& x, const TimeDuration& y);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
#  include "TimeDuration.inl"
#endif

#endif
