#ifndef OPENDDS_DCPS_OPTIONAL_H
#define OPENDDS_DCPS_OPTIONAL_H

#include "Definitions.h"

#include <dds/Versioned_Namespace.h>

#include <ace/config-lite.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

#ifdef ACE_HAS_CPP17
#  define OPENDDS_HAS_STD_OPTIONAL
#endif

#ifdef OPENDDS_HAS_STD_OPTIONAL
#  include <optional>
#  define OPENDDS_OPTIONAL_NS std
#else
#  define OPENDDS_OPTIONAL_NS OpenDDS::DCPS
#endif

#include "SafeBool_T.h"
#include <ace/CDR_Base.h>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class optional : public SafeBool_T<optional<T> > {
public:
  typedef T value_type;

  optional()
    : value_(0)
  {}

  optional(const T& a_value)
    : value_(new(value_storage_) T(a_value))
  {}

  optional(const optional& other)
  {
    if (other.has_value()) {
      value_ = new(value_storage_) T(*other);
    } else {
      value_ = 0;
    }
  }

  template <class U>
  optional(const optional<U>& other)
  {
    if (other.has_value()) {
      value_ = new(value_storage_) T(*other);
    } else {
      value_ = 0;
    }
  }

  ~optional()
  {
    reset();
  }

  optional& operator=(const T& a_value)
  {
    reset();
    value_ = new(value_storage_) T(a_value);
    return *this;
  }

  optional& operator=(const optional& other)
  {
    reset();
    if (other.has_value()) {
      value_ = new(value_storage_) T(*other);
    }
    return *this;
  }

  template <class U>
  optional& operator=(const optional<U>& other)
  {
    reset();
    if (other.has_value()) {
      value_ = new(value_storage_) T(*other);
    }
    return *this;
  }

  const T* operator->() const
  {
    return value_;
  }

  T* operator->()
  {
    return value_;
  }

  const T& operator*() const
  {
    return *value_;
  }

  T& operator*()
  {
    return *value_;
  }

  bool has_value() const
  {
    return boolean_test();
  }

  T& value()
  {
    if (!has_value()) {
      throw std::runtime_error("bad_optional_access");
    }
    return *value_;
  }

  const T& value() const
  {
    if (!has_value()) {
      throw std::runtime_error("bad_optional_access");
    }
    return *value_;
  }

  T value_or(const T& default_value) const
  {
    return has_value() ? value() : default_value;
  }

  void swap(optional& other)
  {
    if (has_value() && other.has_value()) {
      std::swap(**this, *other);
    } else if (has_value()) {
      other.value_ = new(other.value_storage_) T(*value_);
      reset();
    } else if (other.has_value()) {
      value_ = new(value_storage_) T(*other.value_);
      other.reset();
    }
  }

  void reset()
  {
    if (value_) {
      value_->~T();
      value_ = 0;
    }
  }

  bool boolean_test() const
  {
    return value_ != 0;
  }

private:
  T* value_;
  union {
    ACE_CDR::ULongLong max_alignment_;
    unsigned char value_storage_[sizeof(T)];
  };
};

template <typename T>
void swap(optional<T>& lhs, optional<T>& rhs)
{
  return lhs.swap(rhs);
}

template <typename T, typename U>
bool operator==(const optional<T>& lhs, const optional<U>& rhs)
{
  return (!lhs.has_value() && !rhs.has_value()) || (lhs.has_value() && rhs.has_value() && *lhs == *rhs);
}

template <typename T, typename U>
bool operator<(const optional<T>& lhs, const optional<U>& rhs)
{
  return (!lhs.has_value() && rhs.has_value()) || (lhs.has_value() && rhs.has_value() && *lhs < *rhs);
}

template <typename T, typename U>
bool operator==(const optional<T>& lhs, const U& rhs)
{
  return lhs.has_value() && *lhs == rhs;
}

template <typename T, typename U>
bool operator==(const T& lhs, const optional<U>& rhs)
{
  return rhs.has_value() && lhs == *rhs;
}

template <typename T, typename U>
bool operator<(const optional<T>& lhs, const U& rhs)
{
  return !lhs.has_value() || *lhs < rhs;
}

template <typename T, typename U>
bool operator<(const T& lhs, const optional<U>& rhs)
{
  return rhs.has_value() && lhs < *rhs;
}

template <typename T, typename U>
bool operator!=(const optional<T>& lhs, const optional<U>& rhs)
{
  return (lhs.has_value() != rhs.has_value()) || (lhs.has_value() && rhs.has_value() && !(*lhs == *rhs));
}

template <typename T, typename U>
bool operator<=(const optional<T>& lhs, const optional<U>& rhs)
{
  return !lhs.has_value() || (lhs.has_value() && rhs.has_value() && !(*rhs < *lhs));
}

template <typename T, typename U>
bool operator>(const optional<T>& lhs, const optional<U>& rhs)
{
  return (lhs.has_value() && !rhs.has_value()) || (lhs.has_value() && rhs.has_value() && *rhs < *lhs);
}

template <typename T, typename U>
bool operator>=(const optional<T>& lhs, const optional<U>& rhs)
{
  return !rhs.has_value() || (lhs.has_value() && rhs.has_value() && !(*lhs < *rhs));
}

template <typename T, typename U>
bool operator!=(const optional<T>& lhs, const U& rhs)
{
  return !lhs.has_value() || !(*lhs == rhs);
}

template <typename T, typename U>
bool operator!=(const T& lhs, const optional<U>& rhs)
{
  return !rhs.has_value() || !(lhs == *rhs);
}

template <typename T, typename U>
bool operator<=(const optional<T>& lhs, const U& rhs)
{
  return !lhs.has_value() || !(rhs < *lhs);
}

template <typename T, typename U>
bool operator<=(const T& lhs, const optional<U>& rhs)
{
  return rhs.has_value() && !(*rhs < lhs);
}

template <typename T, typename U>
bool operator>(const optional<T>& lhs, const U& rhs)
{
  return lhs.has_value() && (rhs < *lhs);
}

template <typename T, typename U>
bool operator>(const T& lhs, const optional<U>& rhs)
{
  return rhs.has_value() && !(*rhs < lhs);
}

template <typename T, typename U>
bool operator>=(const optional<T>& lhs, const U& rhs)
{
  return lhs.has_value() && !(*lhs < rhs);
}

template <typename T, typename U>
bool operator>=(const T& lhs, const optional<U>& rhs)
{
  return rhs.has_value() && !(*rhs < lhs);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
