#ifndef OPENDDS_DCPS_TIMEPOINT_T_H
#define OPENDDS_DCPS_TIMEPOINT_T_H

#include "TimeDuration.h"
#include "SafeBool_T.h"

#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/Time_Value_T.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Represents a specific time in reference to a ACE_Time_Policy and is based on
 * C++11 std::chrono::time_point.
 *
 * This wraps an ACE_Time_Value, but is designed to enforce the logic that this
 * represents a specific point in time. For example, you can't directly add two
 * TimePoints like you can add two ACE_Time_Values, because that wouldn't make
 * sense. Following this theme, all the constructors are to be explicit to
 * avoid careless implicit conversions.
 *
 * See https://opendds.readthedocs.io/en/master/internal/dev_guidelines.html#time
 * (or docs/internal/dev_guidelines.rst) for background and reasoning for this
 * class.
 */
template<typename AceClock>
class TimePoint_T : public SafeBool_T<TimePoint_T<AceClock> > {
public:
  typedef AceClock ClockType;
  typedef ACE_Time_Value_T<AceClock> ValueType;

  const static TimePoint_T<AceClock> zero_value;
  const static TimePoint_T<AceClock> max_value;

  /**
   * Set to zero_value, which is equal to the epoch (the starting point) of the
   * AceClock.
   */
  TimePoint_T();

  /**
   * Assign the value directly to the inner value doing conversions as
   * necessary.
   *
   * \warning Use these carefully. Make sure ACE_Time_Value is a point in time
   * and of the right clock type. DDS::Time_t should be in system clock time
   * and so shouldn't be used with MonotonicTimePoint.
   */
  ///@{
  explicit TimePoint_T(const ACE_Time_Value& ace_time_value);
  explicit TimePoint_T(const ACE_Time_Value_T<AceClock>& ace_time_value);
  explicit TimePoint_T(const DDS::Time_t& dds_time);
  ///@}

  /**
   * Get a TimePoint representing the current time of the AceClock.
   */
  static TimePoint_T<AceClock> now();

  /**
   * Get and set the inner ACE_Time_Value based value. Use value() to pass this
   * into ACE code that requires a point in time matching this ones clock
   * type.
   */
  ///@{
  const ACE_Time_Value_T<AceClock>& value() const;
  void value(const ACE_Time_Value_T<AceClock>& ace_time_value);
  ///@}

  /**
   * Is the object equal to zero_value?
   */
  bool is_zero() const;

  bool boolean_test() const
  {
    return *this != zero_value;
  }

  /**
   * Is the object equal to the maximum possible value, max_value?
   */
  bool is_max() const;

  /**
   * Set the value to the current time of the AceClock Type.
   */
  void set_to_now();

  /**
   * Convert to DDS::Time_t. This is probably only desirable for the system
   * time version of this template class, SystemTimePoint.
   */
  DDS::Time_t to_dds_time() const;

  /**
   * Convert to OpenDDS::DCPS::MonotonicTime_t.
   */
  MonotonicTime_t to_monotonic_time() const;

  TimePoint_T<AceClock>& operator+=(const TimeDuration& td);
  TimePoint_T<AceClock>& operator-=(const TimeDuration& td);

protected:
  /**
   * Strangely, ACE_Time_Policy must be objects to get their time values, so
   * keep a static object.
   */
  static ClockType clock;

  ValueType value_;
};

template<typename AceClock>
TimePoint_T<AceClock> operator+(const TimeDuration& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
TimePoint_T<AceClock> operator+(const TimePoint_T<AceClock>& x, const TimeDuration& y);

template<typename AceClock>
TimeDuration operator-(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
TimePoint_T<AceClock> operator-(const TimePoint_T<AceClock>& x, const TimeDuration& y);

template<typename AceClock>
bool operator<(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
bool operator>(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
bool operator<=(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
bool operator>=(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
bool operator==(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

template<typename AceClock>
bool operator!=(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#  include "TimePoint_T.inl"
#endif /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#  include "TimePoint_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#  pragma implementation ("TimePoint_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif
