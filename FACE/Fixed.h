#ifndef FACE_FIXED_HEADER_FILE
#define FACE_FIXED_HEADER_FILE

#include "types.hpp"

#include "dds/DCPS/Serializer.h"

#include "tao/SystemException.h"

#include <ace/CDR_Base.h>

namespace OpenDDS {
namespace FaceTypes {

/// General fixed-point decimal type based on the IDL-to-C++ mapping spec v1.3
/// See OMG 2012-07-02 section 5.13
class Fixed
{
public:
  Fixed(int val = 0);
  Fixed(unsigned int val);
  Fixed(FACE::LongLong val);
  Fixed(FACE::UnsignedLongLong val);
  Fixed(FACE::Double val);
  Fixed(FACE::LongDouble val);
  Fixed(const char* str);

  operator FACE::LongLong() const;
  operator FACE::LongDouble() const;
  Fixed round(FACE::UnsignedShort scale) const;
  Fixed truncate(FACE::UnsignedShort scale) const;
  FACE::Char* to_string() const;

  Fixed& operator+=(const Fixed& rhs);
  Fixed& operator-=(const Fixed& rhs);
  Fixed& operator*=(const Fixed& rhs);
  Fixed& operator/=(const Fixed& rhs);

  Fixed& operator++();
  Fixed operator++(int);
  Fixed& operator--();
  Fixed operator--(int);

  Fixed operator+() const;
  Fixed operator-() const;
  bool operator!() const;

  FACE::UnsignedShort fixed_digits() const;
  FACE::UnsignedShort fixed_scale() const;

#ifndef ACE_LACKS_IOSTREAM_TOTALLY
  friend std::istream& operator>>(std::istream& is, Fixed& val)
  {
    return is >> val.impl_;
  }
#endif

  friend ACE_OSTREAM_TYPE& operator<<(ACE_OSTREAM_TYPE& os, const Fixed& val)
  {
    return os << val.impl_;
  }

  friend Fixed operator+(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ + rhs.impl_;
  }

  friend Fixed operator-(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ - rhs.impl_;
  }

  friend Fixed operator*(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ * rhs.impl_;
  }

  friend Fixed operator/(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ / rhs.impl_;
  }

  friend bool operator<(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ < rhs.impl_;
  }

  friend bool operator>(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ > rhs.impl_;
  }

  friend bool operator<=(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ <= rhs.impl_;
  }

  friend bool operator>=(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ >= rhs.impl_;
  }

  friend bool operator==(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ == rhs.impl_;
  }

  friend bool operator!=(const Fixed& lhs, const Fixed& rhs)
  {
    return lhs.impl_ != rhs.impl_;
  }

protected:
#ifdef ACE_HAS_CDR_FIXED
  ACE_CDR::Fixed impl_;

  Fixed(const ACE_CDR::Fixed& f)
    : impl_(f)
  {}
#endif

  static FACE::LongDouble doubleToLongDouble(FACE::Double d)
  {
    FACE::LongDouble ld;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld, d);
    return ld;
  }
};


inline Fixed::Fixed(int val)
  : impl_(ACE_CDR::Fixed::from_integer(FACE::LongLong(val)))
{}

inline Fixed::Fixed(unsigned int val)
  : impl_(ACE_CDR::Fixed::from_integer(FACE::UnsignedLongLong(val)))
{}

inline Fixed::Fixed(FACE::LongLong val)
  : impl_(ACE_CDR::Fixed::from_integer(val))
{}

inline Fixed::Fixed(FACE::UnsignedLongLong val)
  : impl_(ACE_CDR::Fixed::from_integer(val))
{}

inline Fixed::Fixed(FACE::Double val)
  : impl_(ACE_CDR::Fixed::from_floating(doubleToLongDouble(val)))
{}

inline Fixed::Fixed(FACE::LongDouble val)
  : impl_(ACE_CDR::Fixed::from_floating(val))
{}

inline Fixed::Fixed(const char* str)
  : impl_(ACE_CDR::Fixed::from_string(str))
{}

inline Fixed::operator FACE::LongLong() const { return impl_; }

inline Fixed::operator FACE::LongDouble() const { return impl_; }

inline Fixed Fixed::round(FACE::UnsignedShort scale) const
{
  return impl_.round(scale);
}

inline Fixed Fixed::truncate(FACE::UnsignedShort scale) const
{
  return impl_.truncate(scale);
}

inline FACE::Char* Fixed::to_string() const
{
  char buf[ACE_CDR::Fixed::MAX_STRING_SIZE];
  impl_.to_string(buf, sizeof buf);
  return FACE::string_dup(buf);
}

inline Fixed& Fixed::operator+=(const Fixed& rhs)
{
  impl_ += rhs.impl_;
  return *this;
}

inline Fixed& Fixed::operator-=(const Fixed& rhs)
{
  impl_ -= rhs.impl_;
  return *this;
}

inline Fixed& Fixed::operator*=(const Fixed& rhs)
{
  impl_ *= rhs.impl_;
  return *this;
}

inline Fixed& Fixed::operator/=(const Fixed& rhs)
{
  impl_ /= rhs.impl_;
  return *this;
}

inline Fixed& Fixed::operator++()
{
  ++impl_;
  return *this;
}

inline Fixed Fixed::operator++(int)
{
  const Fixed cpy(*this);
  ++impl_;
  return cpy;
}

inline Fixed& Fixed::operator--()
{
  --impl_;
  return *this;
}

inline Fixed Fixed::operator--(int)
{
  const Fixed cpy(*this);
  --impl_;
  return cpy;
}

inline Fixed Fixed::operator+() const { return *this; }

inline Fixed Fixed::operator-() const { return -impl_; }

inline bool Fixed::operator!() const { return !impl_; }

inline FACE::UnsignedShort Fixed::fixed_digits() const
{
  return impl_.fixed_digits();
}

inline FACE::UnsignedShort Fixed::fixed_scale() const
{
  return impl_.fixed_scale();
}


/// Fixed-point decimal type with Digits and Scale set at compile time
template <unsigned int Digits, unsigned int Scale>
class Fixed_T : public Fixed
{
public:
  Fixed_T(int val = 0);
  Fixed_T(unsigned int val);
  Fixed_T(FACE::LongLong val);
  Fixed_T(FACE::UnsignedLongLong val);
  Fixed_T(FACE::Double val);
  Fixed_T(FACE::LongDouble val);
  Fixed_T(const char* str);
  Fixed_T(const Fixed& f);

private:
  friend bool operator<<(DCPS::Serializer& ser, const Fixed_T& f)
  {
#ifdef ACE_HAS_CDR_FIXED
    const int sz = (static_cast<int>(Digits) + 2) / 2;
    int n;
    const FACE::Octet* arr = f.impl_.to_octets(n);
    if (n < sz) {
      arr -= sz - n;
    }
    return ser.write_octet_array(arr, sz);
#else
    ACE_UNUSED_ARG(ser);
    ACE_UNUSED_ARG(f);
    return false;
#endif
  }

  friend bool operator>>(DCPS::Serializer& ser, Fixed_T& f)
  {
#ifdef ACE_HAS_CDR_FIXED
    static const unsigned int n = (Digits + 2) / 2;
    FACE::Octet arr[n];
    if (!ser.read_octet_array(arr, n)) {
      return false;
    }
    f.impl_ = ACE_CDR::Fixed::from_octets(arr, n, Scale);
    return true;
#else
    ACE_UNUSED_ARG(ser);
    ACE_UNUSED_ARG(f);
    return false;
#endif
  }
};


template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(int val)
  : Fixed(ACE_CDR::Fixed::from_integer(FACE::LongLong(val)))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(unsigned int val)
  : Fixed(ACE_CDR::Fixed::from_integer(FACE::UnsignedLongLong(val)))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(FACE::LongLong val)
  : Fixed(ACE_CDR::Fixed::from_integer(val))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(FACE::UnsignedLongLong val)
  : Fixed(ACE_CDR::Fixed::from_integer(val))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(FACE::Double val)
  : Fixed(ACE_CDR::Fixed::from_floating(doubleToLongDouble(val)))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(FACE::LongDouble val)
  : Fixed(ACE_CDR::Fixed::from_floating(val))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(const char* str)
  : Fixed(ACE_CDR::Fixed::from_string(str))
{}

template <unsigned int Digits, unsigned int Scale>
inline Fixed_T<Digits, Scale>::Fixed_T(const Fixed& f)
  : Fixed(f.truncate(Scale))
{
  if (impl_.fixed_digits() > Digits) {
    throw CORBA::DATA_CONVERSION();
  }
}

template <unsigned int Digits, unsigned int Scale>
inline void
gen_find_size(const Fixed_T<Digits, Scale>&, size_t& size, size_t&)
{
  size += (Digits + 2) / 2;
}

template <unsigned int Digits, unsigned int Scale>
inline void
gen_skip_over(DCPS::Serializer& ser, Fixed_T<Digits, Scale>*)
{
  ser.skip((Digits + 2) / 2);
}

}
}

namespace FACE {
  typedef OpenDDS::FaceTypes::Fixed Fixed;
}

#endif
