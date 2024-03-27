/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SafetyProfileStreams.h"

#include "ace/OS_NS_string.h"
#include "ace/Truncate.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// These operators are used in some inline functions below.  Some
// compilers require the inline definition to appear before its use.
#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE
bool operator==(const DDS::Duration_t& t1, const DDS::Duration_t& t2)
{
  return t1.sec == t2.sec && t1.nanosec == t2.nanosec;
}

ACE_INLINE
bool operator!=(const DDS::Duration_t& t1, const DDS::Duration_t& t2)
{
  return !(t1 == t2);
}
#endif

ACE_INLINE
bool operator<(const DDS::Duration_t& t1, const DDS::Duration_t& t2)
{
  // @note We wouldn't have to handle the case for INFINITY explicitly
  //       if both the Duration_t sec and nanosec fields were the
  //       maximum values for their corresponding types.
  //       Unfortunately, the OMG DDS specification defines the
  //       infinite nanosec value to be somewhere in the middle.

  // We assume that either both the DDS::Duration_t::sec and
  // DDS::Duration_t::nanosec fields are INFINITY or neither of them
  // are.  It doesn't make sense for only one of the fields to be
  // INFINITY.
  return
    !is_infinite(t1)
    && (is_infinite(t2)
        || t1.sec < t2.sec
        || (t1.sec == t2.sec && t1.nanosec < t2.nanosec));
}

ACE_INLINE
bool operator<=(const DDS::Duration_t& t1, const DDS::Duration_t& t2)
{
  // If t2 is *not* less than t1, t1 must be less than
  // or equal to t2.
  // This is more concise than:
  //   t1 < t2 || t1 == t2
  return !(t2 < t1);
}

ACE_INLINE
bool operator>(const DDS::Duration_t& t1, const DDS::Duration_t& t2)
{
  return t2 < t1;
}

ACE_INLINE
bool operator>=(const DDS::Duration_t& t1, const DDS::Duration_t& t2)
{
  return t2 <= t1;
}

ACE_INLINE
bool operator!(const DDS::Time_t& t)
{
  return t.sec == DDS::TIME_INVALID_SEC
         || t.nanosec == DDS::TIME_INVALID_NSEC;
}

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE bool
operator==(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  return !(t1 < t2) && !(t2 < t1);
}

ACE_INLINE bool
operator!=(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  return !(t1 == t2);
}
#endif

ACE_INLINE bool
operator<(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  if (!t1 || !t2) return false;

  return t1.sec < t2.sec
         || (t1.sec == t2.sec && t1.nanosec < t2.nanosec);
}

ACE_INLINE bool
operator<=(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  return !(t2 < t1);
}

ACE_INLINE bool
operator>(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  return t2 < t1;
}

ACE_INLINE bool
operator>=(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  return t2 <= t1;
}

ACE_INLINE DDS::Time_t
operator+(const DDS::Time_t& t1, const DDS::Duration_t& d1)
{
  CORBA::LongLong sec = t1.sec + d1.sec;
  CORBA::ULong nanosec = t1.nanosec + d1.nanosec;

  while (nanosec >= ACE_ONE_SECOND_IN_NSECS) {
    ++sec;
    nanosec -= ACE_ONE_SECOND_IN_NSECS;
  }

  const DDS::Time_t t = { sec, nanosec };
  return t;
}

ACE_INLINE DDS::Duration_t
operator-(const DDS::Time_t& t1, const DDS::Time_t& t2)
{
  DDS::Duration_t t = { static_cast<CORBA::Long>(t1.sec - t2.sec), t1.nanosec - t2.nanosec };

  if (t2.nanosec > t1.nanosec) {
      t.nanosec = (t1.nanosec + ACE_ONE_SECOND_IN_NSECS) - t2.nanosec;
      t.sec = (t1.sec - 1) - t2.sec;
    }

  return t;
}

ACE_INLINE DDS::Time_t
operator-(const DDS::Time_t& t1, const DDS::Duration_t& t2)
{
  DDS::Time_t t = { t1.sec - t2.sec, t1.nanosec - t2.nanosec };

  if (t2.nanosec > t1.nanosec) {
      t.nanosec = (t1.nanosec + ACE_ONE_SECOND_IN_NSECS) - t2.nanosec;
      t.sec = (t1.sec - 1) - t2.sec;
    }

  return t;
}

ACE_INLINE DDS::Duration_t
operator-(const MonotonicTime_t& t1, const MonotonicTime_t& t2)
{
  DDS::Duration_t t = { static_cast<CORBA::Long>(t1.sec - t2.sec), t1.nanosec - t2.nanosec };

  if (t2.nanosec > t1.nanosec) {
      t.nanosec = (t1.nanosec + ACE_ONE_SECOND_IN_NSECS) - t2.nanosec;
      t.sec = (t1.sec - 1) - t2.sec;
    }

  return t;
}

ACE_INLINE bool
operator<(const MonotonicTime_t& t1, const MonotonicTime_t& t2)
{
  return t1.sec < t2.sec || (t1.sec == t2.sec && t1.nanosec < t2.nanosec);
}

#ifndef OPENDDS_SAFETY_PROFILE
ACE_INLINE bool
operator==(const MonotonicTime_t& t1, const MonotonicTime_t& t2)
{
  return t1.sec == t2.sec && t1.nanosec == t2.nanosec;
}
#endif

ACE_INLINE
ACE_Time_Value time_to_time_value(const DDS::Time_t& t)
{
  ACE_Time_Value tv(t.sec, t.nanosec / 1000);
  return tv;
}

ACE_INLINE
DDS::Time_t time_value_to_time(const ACE_Time_Value& tv)
{
  DDS::Time_t t;
  t.sec = tv.sec();
  t.nanosec = ACE_Utils::truncate_cast<CORBA::ULong>(tv.usec() * 1000);
  return t;
}

ACE_INLINE
MonotonicTime_t time_value_to_monotonic_time(const ACE_Time_Value& tv)
{
  MonotonicTime_t t;
  t.sec = tv.sec();
  t.nanosec = ACE_Utils::truncate_cast<CORBA::ULong>(tv.usec() * 1000);
  return t;
}

ACE_INLINE
ACE_Time_Value duration_to_time_value(const DDS::Duration_t& t)
{
  if (is_infinite(t)) {
    return ACE_Time_Value::max_time;
  }

  CORBA::LongLong sec = t.sec + t.nanosec/1000/ACE_ONE_SECOND_IN_USECS;
  CORBA::ULong usec = t.nanosec/1000 % ACE_ONE_SECOND_IN_USECS;

  if (sec > ACE_Time_Value::max_time.sec()) {
    return ACE_Time_Value::max_time;
  }
  else {
    return ACE_Time_Value(ACE_Utils::truncate_cast<time_t>(sec), usec);
  }
}

ACE_INLINE
ACE_Time_Value duration_to_absolute_time_value(const DDS::Duration_t& t,
                                               const ACE_Time_Value& now)
{
  CORBA::LongLong sec
    = t.sec + now.sec() + (t.nanosec/1000 + now.usec())/ACE_ONE_SECOND_IN_USECS;
  CORBA::ULong usec = (t.nanosec/1000 + now.usec()) % ACE_ONE_SECOND_IN_USECS;

  if (sec > ACE_Time_Value::max_time.sec()) {
    return ACE_Time_Value::max_time;
  }
  else {
    return ACE_Time_Value(ACE_Utils::truncate_cast<time_t>(sec), usec);
  }
}

ACE_INLINE
DDS::Duration_t time_value_to_duration(const ACE_Time_Value& tv)
{
  DDS::Duration_t t;
  t.sec = ACE_Utils::truncate_cast<CORBA::Long>(tv.sec());
  t.nanosec = ACE_Utils::truncate_cast<CORBA::ULong>(tv.usec() * 1000);
  return t;
}

ACE_INLINE
double time_value_to_double(const ACE_Time_Value& tv)
{
  return double(tv.sec()) + (double(tv.usec()) / 1000000.0);
}

ACE_INLINE
DDS::Duration_t time_to_duration(const DDS::Time_t& t)
{
  DDS::Duration_t d = { static_cast<CORBA::Long>(t.sec), t.nanosec };
  return d;
}

ACE_INLINE
bool valid_duration(const DDS::Duration_t& t)
{
  // Only accept infinite or positive finite durations.  (Zero
  // excluded).
  //
  // Note that it doesn't make much sense for users to set
  // durations less than 10 milliseconds since the underlying
  // timer resolution is generally no better than that.
  return is_infinite(t) || t.sec > 0 || (t.sec >= 0 && t.nanosec > 0);
}

ACE_INLINE
bool non_negative_duration(const DDS::Duration_t& t)
{
  return
    (t.sec == DDS::DURATION_ZERO_SEC  // Allow zero duration.
     && t.nanosec == DDS::DURATION_ZERO_NSEC)
    || valid_duration(t);
}

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 uint32_fractional_seconds_to_nanoseconds(ACE_UINT32 fraction)
{
  return static_cast<ACE_UINT32>((static_cast<ACE_UINT64>(fraction) * 1000000000) >> 32);
}

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 nanoseconds_to_uint32_fractional_seconds(ACE_UINT32 nsec)
{
  return static_cast<ACE_UINT32>((static_cast<ACE_UINT64>(nsec) << 32) / 1000000000);
}

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 uint32_fractional_seconds_to_microseconds(ACE_UINT32 fraction)
{
  return static_cast<ACE_UINT32>((static_cast<ACE_UINT64>(fraction) * 1000000) >> 32);
}

ACE_INLINE OpenDDS_Dcps_Export
ACE_UINT32 microseconds_to_uint32_fractional_seconds(ACE_UINT32 usec)
{
  return static_cast<ACE_UINT32>((static_cast<ACE_UINT64>(usec) << 32) / 1000000);
}

bool is_infinite(const DDS::Duration_t& value)
{
  return value.sec == DDS::DURATION_INFINITE_SEC &&
    value.nanosec == DDS::DURATION_INFINITE_NSEC;
}

ACE_INLINE OpenDDS_Dcps_Export
const MonotonicTime_t& monotonic_time_zero()
{
  static const MonotonicTime_t zero = { 0, 0 };
  return zero;
}

ACE_INLINE OpenDDS_Dcps_Export
DDS::Duration_t make_duration_t(int sec, unsigned long nanosec)
{
  DDS::Duration_t x;
  x.sec = sec;
  x.nanosec = nanosec;
  return x;
}

namespace {

  String left_pad(const String& x, char p, size_t total_length) {
    return String(total_length - x.size(), p) + x;
  }

  String right_pad(const String& x, char p, size_t total_length) {
    return x + String(total_length - x.size(), p);
  }

}


ACE_INLINE OpenDDS_Dcps_Export
String to_dds_string(const DDS::Duration_t& duration)
{
  if (is_infinite(duration)) {
    return "DURATION_INFINITY";
  }

  return to_dds_string(duration.sec) + '.' + left_pad(to_dds_string(duration.nanosec), '0', 9);
}

ACE_INLINE OpenDDS_Dcps_Export
bool from_dds_string(const String& str, DDS::Duration_t& duration)
{
  if (str == "DURATION_INFINITY") {
    duration.sec = DDS::DURATION_INFINITE_SEC;
    duration.nanosec = DDS::DURATION_INFINITE_NSEC;
    return true;
  }

  DDS::Duration_t temp = { 0, 0 };

  const size_t decimal_pos = str.find_first_of('.');
  if (decimal_pos == std::string::npos) {
    // Assume integer seconds.
    if (!DCPS::convertToInteger(str, temp.sec)) {
      return false;
    }
  } else {
    // Assume decimal seconds.
    const String sec_str = str.substr(0, decimal_pos);
    String nanosec_str = str.substr(decimal_pos + 1);
    if (sec_str.empty() && nanosec_str.empty()) {
      return false;
    }
    if (nanosec_str.size() > 9) {
      return false;
    }
    // Pad the nanosecs.
    nanosec_str = right_pad(nanosec_str, '0', 9);

    if (!sec_str.empty() && !DCPS::convertToInteger(sec_str, temp.sec)) {
      return false;
    }
    if (!nanosec_str.empty() && !DCPS::convertToInteger(nanosec_str, temp.nanosec)) {
      return false;
    }
  }

  duration = temp;
  return true;
}

ACE_INLINE OpenDDS_Dcps_Export
DDS::Time_t make_time_t(int sec, unsigned long nanosec)
{
  DDS::Time_t x;
  x.sec = sec;
  x.nanosec = nanosec;
  return x;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
