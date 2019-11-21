#include "TimeDuration.h"

#include "Time_Helper.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
TimeDuration
TimeDuration::from_msec(const ACE_UINT64& ms)
{
  ACE_Time_Value rv;
  rv.set_msec(ms);
  return TimeDuration(rv);
}

ACE_INLINE
TimeDuration::TimeDuration()
: value_(zero_value.value())
{
}

ACE_INLINE
TimeDuration::TimeDuration(const TimeDuration& other)
: value_(other.value_)
{
}

ACE_INLINE
TimeDuration::TimeDuration(const ACE_Time_Value& ace_time_value)
: value_(ace_time_value)
{
}

ACE_INLINE
TimeDuration::TimeDuration(time_t sec, suseconds_t usec)
: value_(sec, usec)
{
}

ACE_INLINE
TimeDuration::TimeDuration(const DDS::Duration_t& dds_duration)
: value_(duration_to_time_value(dds_duration))
{
}

ACE_INLINE
const ACE_Time_Value&
TimeDuration::value() const
{
  return value_;
}

ACE_INLINE
void
TimeDuration::value(const ACE_Time_Value& ace_time_value)
{
  value_ = ace_time_value;
}

ACE_INLINE
bool
TimeDuration::is_zero() const
{
  return *this == zero_value;
}

ACE_INLINE
bool
TimeDuration::is_max() const
{
  return *this == max_value;
}

ACE_INLINE
DDS::Duration_t
TimeDuration::to_dds_duration() const
{
  return time_value_to_duration(value());
}

ACE_INLINE
TimeDuration&
TimeDuration::operator+=(const TimeDuration& other)
{
  value(value_ + other.value());
  return *this;
}

ACE_INLINE
TimeDuration&
TimeDuration::operator-=(const TimeDuration& other)
{
  value(value_ - other.value());
  return *this;
}

ACE_INLINE
TimeDuration&
TimeDuration::operator*=(double other)
{
  value(value_ * other);
  return *this;
}

ACE_INLINE
TimeDuration&
TimeDuration::operator=(const TimeDuration& other)
{
  value(other.value());
  return *this;
}

ACE_INLINE
TimeDuration&
TimeDuration::operator=(const time_t& other)
{
  value_ = other;
  return *this;
}

ACE_INLINE
TimeDuration
operator+(const TimeDuration& x, const TimeDuration& y)
{
  return TimeDuration(x.value() + y.value());
}

ACE_INLINE
TimeDuration
operator-(const TimeDuration& x, const TimeDuration& y)
{
  return TimeDuration(x.value() - y.value());
}

ACE_INLINE
TimeDuration
operator-(const TimeDuration& x)
{
  return TimeDuration::zero_value - x;
}

ACE_INLINE
TimeDuration
operator*(double x, const TimeDuration& y)
{
  return TimeDuration(x * y.value());
}

ACE_INLINE
TimeDuration
operator*(const TimeDuration& x, double y)
{
  return TimeDuration(x.value() * y);
}

ACE_INLINE
bool
operator<(const TimeDuration& x, const TimeDuration& y)
{
  return x.value() < y.value();
}

ACE_INLINE
bool
operator>(const TimeDuration& x, const TimeDuration& y)
{
  return x.value() > y.value();
}

ACE_INLINE
bool
operator<=(const TimeDuration& x, const TimeDuration& y)
{
  return x.value() <= y.value();
}

ACE_INLINE
bool
operator>=(const TimeDuration& x, const TimeDuration& y)
{
  return x.value() >= y.value();
}

ACE_INLINE
bool
operator==(const TimeDuration& x, const TimeDuration& y)
{
  return x.value() == y.value();
}

ACE_INLINE
bool
operator!=(const TimeDuration& x, const TimeDuration& y)
{
  return x.value() != y.value();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
