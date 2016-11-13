/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RCEVENTHANDLER_H
#define OPENDDS_RCEVENTHANDLER_H

#include "ace/Event_Handler.h"

namespace OpenDDS {
namespace DCPS {

/// Templated Reference counted handle to a pointer.
/// A non-DDS specific helper class.
template <typename T>
class RcEventHandler {
public:

  RcEventHandler()
    : ptr_(0)
  {}

  explicit RcEventHandler(T* p)
    : ptr_(p)
  {
    if (p)
      p->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
  }

  RcEventHandler(const RcEventHandler& b)
    : ptr_(b.ptr_)
  {
    this->bump_up();
  }

  ~RcEventHandler()
  {
    this->bump_down();
  }

  void reset(T* p=0)
  {
    RcEventHandler tmp(p);
    swap(tmp);
  }

  RcEventHandler& operator=(const RcEventHandler& b)
  {
    RcEventHandler tmp(b);
    swap(tmp);
    return *this;
  }

  void swap(RcEventHandler& rhs)
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

  bool operator == (T* p) const {
    return this->ptr_ == p;
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

  bool operator==(const RcEventHandler& rhs)
  {
    return in() == rhs.in();
  }

  bool operator!=(const RcEventHandler& rhs)
  {
    return in() != rhs.in();
  }

private:

  void bump_up()
  {
    if (this->ptr_ != 0) {
      this->ptr_->add_reference();
    }
  }

  void bump_down()
  {
    if (this->ptr_ != 0) {
      this->ptr_->remove_reference();
      this->ptr_ = 0;
    }
  }

  /// The actual "unsmart" pointer to the T object.
  T* ptr_;
};


template <typename T>
void swap(RcEventHandler<T>& lhs, RcEventHandler<T>& rhs)
{
  lhs.swap(rhs);
}


} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_RCEVENTHANDLER_H */
