/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RCHANDLE_T_H
#define OPENDDS_RCHANDLE_T_H

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
  
struct inc_count {};
struct keep_count {};

/// Templated Reference counted handle to a pointer.
/// A non-DDS specific helper class.
template <typename T>
class RcHandle {
public:

  RcHandle()
    : ptr_(0)
  {}
    
  RcHandle(T* p, keep_count)
    : ptr_(p)
  {
  }
  

  RcHandle(T* p, inc_count)
    : ptr_(p)
  {
    this->bump_up();
  }

  template <typename U>
  RcHandle(const RcHandle<U>& other)
    : ptr_(other.in())
  {
    this->bump_up();
  }

  RcHandle(const RcHandle& b)
    : ptr_(b.ptr_)
  {
    this->bump_up();
  }

  ~RcHandle()
  {
    this->bump_down();
  }

  void reset()
  {
    RcHandle tmp;
    swap(tmp);
  }

  template <typename U>
  void reset(T* p, U counting_strategy)
  {
    RcHandle tmp(p, counting_strategy);
    swap(tmp);
  }

  RcHandle& operator=(const RcHandle& b)
  {
    RcHandle tmp(b);
    swap(tmp);
    return *this;
  }

  template <class U>
  RcHandle& operator=(const RcHandle<U>& b)
  {
    RcHandle<T> tmp(b);
    swap(tmp);
    return *this;
  }

  void swap(RcHandle& rhs)
  {
    T* t = this->ptr_;
    this->ptr_ = rhs.ptr_;
    rhs.ptr_ = t;
  }

  T* operator->() const
  {
    return this->ptr_;
  }

  T& operator*() const
  {
    return *this->ptr_;
  }

  bool is_nil() const
  {
    return this->ptr_ == 0;
  }

  T* in() const
  {
    return this->ptr_;
  }

  T*& inout()
  {
    return this->ptr_;
  }

  T*& out()
  {
    this->bump_down();
    return this->ptr_;
  }

  T* _retn()
  {
    T* retval = this->ptr_;
    this->ptr_ = 0;
    return retval;
  }


  operator bool() const
  {
    return in() != 0;
  }

  bool operator==(const RcHandle& rhs) const
  {
    return in() == rhs.in();
  }

  bool operator!=(const RcHandle& rhs) const
  {
    return in() != rhs.in();
  }

  bool operator < (const RcHandle& rhs) const
  {
    return in() < rhs.in();
  }

private:

  void bump_up()
  {
    if (this->ptr_ != 0) {
      this->ptr_->_add_ref();
    }
  }

  void bump_down()
  {
    if (this->ptr_ != 0) {
      this->ptr_->_remove_ref();
      this->ptr_ = 0;
    }
  }

  /// The actual "unsmart" pointer to the T object.
  T* ptr_;
};


template <typename T>
void swap(RcHandle<T>& lhs, RcHandle<T>& rhs)
{
  lhs.swap(rhs);
}

template <typename T, typename U>
RcHandle<T> static_rchandle_cast(const RcHandle<U>& h)
{
  return RcHandle<T>(static_cast<T*>(h.in()), inc_count());
}

template <typename T, typename U>
RcHandle<T> const_rchandle_cast(const RcHandle<U>& h)
{
  return RcHandle<T>(const_cast<T*>(h.in()), inc_count());
}

template <typename T, typename U>
RcHandle<T> dynamic_rchandle_cast(const RcHandle<U>& h)
{
  return RcHandle<T>(dynamic_cast<T*>(h.in()), inc_count());
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_RCHANDLE_T_H */
