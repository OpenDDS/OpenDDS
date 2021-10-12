#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "TimeDuration.h"

#include "SafetyProfileStreams.h"

#include <cmath>
#include <cstdio>
#ifdef ACE_HAS_CPP11
#  include <limits>
#endif /* ACE_HAS_CPP11*/

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const TimeDuration TimeDuration::zero_value(0, 0);
#ifdef ACE_HAS_CPP11
const TimeDuration TimeDuration::max_value(std::numeric_limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1);
#else
const TimeDuration TimeDuration::max_value(ACE_Numeric_Limits<time_t>::max(), ACE_ONE_SECOND_IN_USECS - 1);
#endif /* ACE_HAS_CPP11 */

namespace {
  inline unsigned long usec_to_rounded_frac(
    suseconds_t value, unsigned decimal_places, unsigned long& carry)
  {
    const float frac = static_cast<float>(value) / ACE_ONE_SECOND_IN_USECS;
    const float denominator = std::pow(10.0f, decimal_places);
    const unsigned long numerator = std::floor(frac * denominator + 0.5f);
    if (numerator == denominator) {
      carry = 1;
      return 0;
    }
    carry = 0;
    return numerator;
  }

  String to_zero_pad_str(unsigned long value, unsigned len = 2)
  {
    String str(len, '\0');
    std::snprintf(&str[0], str.size() + 1, "%0*lu", len, value);
    return str;
  }
}

String TimeDuration::str(unsigned decimal_places) const
{
  String rv;
  unsigned long carry;
  const unsigned long numerator = usec_to_rounded_frac(value().usec(), decimal_places, carry);
  const unsigned long seconds_total = value().sec() + carry;
  const unsigned long minutes_total = seconds_total / 60;
  const unsigned long seconds = seconds_total % 60;
  if (minutes_total > 0) {
    const unsigned long hours = minutes_total / 60;
    const unsigned long minutes = minutes_total % 60;
    if (hours > 0) {
      rv += to_dds_string(hours) + ":" + to_zero_pad_str(minutes);
    } else {
      rv += to_dds_string(minutes);
    }
    rv += ":" + to_zero_pad_str(seconds);
  } else {
    rv += to_dds_string(seconds);
  }
  if (decimal_places > 0) {
    rv += "." + to_zero_pad_str(numerator, decimal_places);
  }
  if (minutes_total == 0) {
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
