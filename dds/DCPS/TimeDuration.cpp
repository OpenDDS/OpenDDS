#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "TimeDuration.h"

#include "SafetyProfileStreams.h"

#include <cmath>
#include <cstdio>
#ifdef ACE_HAS_CPP11
#  include <limits>
#else
#  include <ace/Numeric_Limits.h>
#endif /* ACE_HAS_CPP11*/

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const TimeDuration TimeDuration::zero_value(0, 0);
const TimeDuration TimeDuration::max_value(
#ifdef ACE_HAS_CPP11
  std::numeric_limits<time_t>::max(),
#else
  ACE_Numeric_Limits<time_t>::max(),
#endif
  ACE_ONE_SECOND_IN_USECS - 1);

namespace {
  time_t usec_to_rounded_frac(
    suseconds_t value, unsigned decimal_places, time_t& carry)
  {
    const double frac = static_cast<double>(value) / ACE_ONE_SECOND_IN_USECS;
    const double denominator = std::pow(10.0, static_cast<double>(decimal_places));
    const double numerator = std::floor(frac * denominator + 0.5);
    if (numerator == denominator) {
      carry = 1;
      return 0;
    }
    carry = 0;
    return static_cast<time_t>(numerator);
  }

  String to_zero_pad_str(time_t value, unsigned len = 2)
  {
    const String nopad = to_dds_string(value);
    if (len > nopad.size()) {
      return String(len - nopad.size(), '0') + nopad;
    }
    return nopad;
  }
}

String TimeDuration::str(unsigned decimal_places, bool just_sec) const
{
  String rv;
  time_t sec = value().sec();
  suseconds_t usec = value().usec();
  bool negative = false;
  if (sec < 0) {
    negative = true;
    sec = -sec;
  }
  if (usec < 0) {
    negative = true;
    usec = -usec;
  }
  if (negative) {
    rv += "-";
  }
  time_t carry;
  const time_t numerator = usec_to_rounded_frac(usec, decimal_places, carry);
  const time_t seconds_total = sec + carry;
  const time_t minutes_total = seconds_total / 60;
  just_sec = just_sec || minutes_total == 0;
  if (just_sec) {
    rv += to_dds_string(seconds_total);
  } else {
    const time_t seconds = seconds_total % 60;
    const time_t minutes = minutes_total % 60;
    const time_t hours = minutes_total / 60;
    if (hours > 0) {
      rv += to_dds_string(hours) + ":" + to_zero_pad_str(minutes);
    } else {
      rv += to_dds_string(minutes);
    }
    rv += ":" + to_zero_pad_str(seconds);
  }
  if (decimal_places > 0) {
    rv += "." + to_zero_pad_str(numerator, decimal_places);
  }
  if (just_sec) {
    rv += " s";
  }
  return rv;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if !defined (__ACE_INLINE__)
#  include "TimeDuration.inl"
#endif /* __ACE_INLINE__ */
