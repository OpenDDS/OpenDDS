#include "TimePoint_T.h"

#include "Time_Helper.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>::TimePoint_T()
: value_(zero_value.value())
{
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>::TimePoint_T(const ACE_Time_Value& ace_time_value)
: value_(ace_time_value)
{
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>::TimePoint_T(const ACE_Time_Value_T<AceClock>& ace_time_value)
: value_(ace_time_value)
{
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>::TimePoint_T(const DDS::Time_t& dds_time)
: value_(time_to_time_value(dds_time))
{
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>
TimePoint_T<AceClock>::now()
{
  return TimePoint_T<AceClock>(clock());
}

template<typename AceClock>
ACE_INLINE
const ACE_Time_Value_T<AceClock>&
TimePoint_T<AceClock>::value() const
{
  return value_;
}

template<typename AceClock>
ACE_INLINE
void
TimePoint_T<AceClock>::value(const ACE_Time_Value_T<AceClock>& ace_time_value)
{
  value_ = ace_time_value;
}

template<typename AceClock>
ACE_INLINE
bool
TimePoint_T<AceClock>::is_zero() const
{
  return *this == zero_value;
}

template<typename AceClock>
ACE_INLINE
bool
TimePoint_T<AceClock>::is_max() const
{
  return *this == max_value;
}

template<typename AceClock>
ACE_INLINE
void
TimePoint_T<AceClock>::set_to_now()
{
  value_ = clock();
}

template<typename AceClock>
ACE_INLINE
DDS::Time_t
TimePoint_T<AceClock>::to_dds_time() const
{
  return time_value_to_time(value_);
}

template<typename AceClock>
ACE_INLINE
MonotonicTime_t
TimePoint_T<AceClock>::to_monotonic_time() const
{
  return time_value_to_monotonic_time(value_);
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>&
TimePoint_T<AceClock>::operator+=(const TimeDuration& td)
{
  value_ += td.value();
  return *this;
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>&
TimePoint_T<AceClock>::operator-=(const TimeDuration& td)
{
  value_ -= td.value();
  return *this;
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>
operator+(const TimeDuration& x, const TimePoint_T<AceClock>& y)
{
  return TimePoint_T<AceClock>(x.value() + y.value());
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>
operator+(const TimePoint_T<AceClock>& x, const TimeDuration& y)
{
  return TimePoint_T<AceClock>(x.value() + y.value());
}

template<typename AceClock>
ACE_INLINE
TimeDuration
operator-(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return TimeDuration(x.value() - y.value());
}

template<typename AceClock>
ACE_INLINE
TimePoint_T<AceClock>
operator-(const TimePoint_T<AceClock>& x, const TimeDuration& y)
{
  return TimePoint_T<AceClock>(x.value() - y.value());
}

template<typename AceClock>
ACE_INLINE
bool
operator<(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return x.value() < y.value();
}

template<typename AceClock>
ACE_INLINE
bool
operator>(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return x.value() > y.value();
}

template<typename AceClock>
ACE_INLINE
bool
operator<=(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return x.value() <= y.value();
}

template<typename AceClock>
ACE_INLINE
bool
operator>=(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return x.value() >= y.value();
}

template<typename AceClock>
ACE_INLINE
bool
operator==(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return x.value() == y.value();
}

template<typename AceClock>
ACE_INLINE
bool
operator!=(const TimePoint_T<AceClock>& x, const TimePoint_T<AceClock>& y)
{
  return x.value() != y.value();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
