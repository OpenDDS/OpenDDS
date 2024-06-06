#include "TimePoint_T.h"

#include "Time_Helper.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>::TimePoint_T()
: value_(zero_value.value())
{
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>::TimePoint_T(const ACE_Time_Value& ace_time_value)
: value_(ace_time_value)
{
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>::TimePoint_T(const ACE_Time_Value_T<AceClock>& ace_time_value)
: value_(ace_time_value)
{
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>::TimePoint_T(const IdlType& idl_time)
: value_(time_to_time_value(idl_time))
{
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>
TimePoint_T<AceClock, IdlType>::now()
{
  return TimePoint_T<AceClock, IdlType>(clock());
}

template<typename AceClock, typename IdlType>
ACE_INLINE
const ACE_Time_Value_T<AceClock>&
TimePoint_T<AceClock, IdlType>::value() const
{
  return value_;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
void
TimePoint_T<AceClock, IdlType>::value(const ACE_Time_Value_T<AceClock>& ace_time_value)
{
  value_ = ace_time_value;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
TimePoint_T<AceClock, IdlType>::is_zero() const
{
  return *this == zero_value;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
TimePoint_T<AceClock, IdlType>::is_max() const
{
  return *this == max_value;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
void
TimePoint_T<AceClock, IdlType>::set_to_now()
{
  value_ = clock();
}

template<typename AceClock, typename IdlType>
ACE_INLINE
IdlType
TimePoint_T<AceClock, IdlType>::to_idl_struct() const
{
  IdlType i;
  time_value_to_time(i, value_);
  return i;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>&
TimePoint_T<AceClock, IdlType>::operator+=(const TimeDuration& td)
{
  value_ += td.value();
  return *this;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>&
TimePoint_T<AceClock, IdlType>::operator-=(const TimeDuration& td)
{
  value_ -= td.value();
  return *this;
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>
operator+(const TimeDuration& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return TimePoint_T<AceClock, IdlType>(x.value() + y.value());
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>
operator+(const TimePoint_T<AceClock, IdlType>& x, const TimeDuration& y)
{
  return TimePoint_T<AceClock, IdlType>(x.value() + y.value());
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimeDuration
operator-(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return TimeDuration(x.value() - y.value());
}

template<typename AceClock, typename IdlType>
ACE_INLINE
TimePoint_T<AceClock, IdlType>
operator-(const TimePoint_T<AceClock, IdlType>& x, const TimeDuration& y)
{
  return TimePoint_T<AceClock, IdlType>(x.value() - y.value());
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
operator<(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return x.value() < y.value();
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
operator>(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return x.value() > y.value();
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
operator<=(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return x.value() <= y.value();
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
operator>=(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return x.value() >= y.value();
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
operator==(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return x.value() == y.value();
}

template<typename AceClock, typename IdlType>
ACE_INLINE
bool
operator!=(const TimePoint_T<AceClock, IdlType>& x, const TimePoint_T<AceClock, IdlType>& y)
{
  return x.value() != y.value();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
