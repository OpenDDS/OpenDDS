/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TAO_DDS_RCHANDLE_T_H
#define TAO_DDS_RCHANDLE_T_H

#include <algorithm>

namespace OpenDDS {
namespace DCPS {

/// Templated Reference counted handle to a pointer.
/// A non-DDS specific helper class.
template <typename T>
class RcHandle {
public:

  RcHandle()
    : ptr_(0)
  {}

  RcHandle(T *p, bool take_ownership = true)
    : ptr_(p)
  {
    if (!take_ownership) {
      this->bump_up();
    }
  }

  RcHandle(const RcHandle &b)
    : ptr_(b.ptr_)
  {
    this->bump_up();
  }

  ~RcHandle() {
    this->bump_down();
  }

  RcHandle& operator=(T* p) {
    if (this->ptr_ != p) {
      RcHandle tmp(p);
      std::swap(this->ptr_, tmp.ptr_);
    }

    return *this;
  }

  RcHandle& operator=(const RcHandle& b) {
    RcHandle tmp(b);
    std::swap(this->ptr_, tmp.ptr_);
    return *this;
  }

  T* operator->() const {
    return this->ptr_;
  }

  unsigned is_nil() const {
    return this->ptr_ == 0;
  }

  T* in() const {
    return this->ptr_;
  }

  T*& inout() {
    return this->ptr_;
  }

  T*& out() {
    this->bump_down();
    return this->ptr_;
  }

  T *_retn() {
    T* retval = this->ptr_;
    this->ptr_ = 0;
    return retval;
  }

  bool operator==(const RcHandle& rhs) {
    return in() == rhs.in();
  }

  bool operator!=(const RcHandle& rhs) {
    return in() != rhs.in();
  }

private:

  void bump_up() {
    if (this->ptr_ != 0) {
      this->ptr_->_add_ref();
    }
  }

  void bump_down() {
    if (this->ptr_ != 0) {
      this->ptr_->_remove_ref();
      this->ptr_ = 0;
    }
  }

  /// The actual "unsmart" pointer to the T object.
  T* ptr_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* TAO_DDS_RCHANDLE_T_H */
