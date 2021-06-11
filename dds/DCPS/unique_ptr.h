#ifndef OPENDDS_DCPS_UNIQUE_PTR_H
#define OPENDDS_DCPS_UNIQUE_PTR_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


#include "dds/Versioned_Namespace.h"

#include "ace/config-lite.h"

#ifdef ACE_HAS_CPP11
#  define OPENDDS_HAS_STD_UNIQUE_PTR
#endif

#ifdef OPENDDS_HAS_STD_UNIQUE_PTR
#  include <memory>
#else
#  include "ace/Atomic_Op.h"
#  include "ace/Synch_Traits.h"
#  ifdef ACE_HAS_CPP11
#    include <utility>
#  else
#    include <algorithm>
#  endif
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef OPENDDS_HAS_STD_UNIQUE_PTR

using std::move;
using std::unique_ptr;

template <typename T>
using container_supported_unique_ptr = std::unique_ptr<T>;

template <typename T>
struct EnableContainerSupportedUniquePtr {};

#else //HAS_STD_UNIQUE_PTR

template <typename T>
class rv : public T {
  rv();
  ~rv();
  rv(const rv &);
  void operator=(const rv&);
};

template <typename T>
struct default_deleter
{
  void operator()(T* ptr) const
  {
    delete ptr;
  }
};

template <typename T, typename Deleter = default_deleter<T> >
class unique_ptr {
public:
  typedef T element_type;
  typedef Deleter deleter_type;

  explicit unique_ptr(T* p = 0) // never throws
    : ptr_(p)
  {}

#ifndef __SUNPRO_CC
  typedef rv<unique_ptr>& rv_reference;
#else
  typedef unique_ptr& rv_reference;
#endif

  unique_ptr(rv_reference other)
    : ptr_(other.release())
  {}

  ~unique_ptr() // never throws
  {
    Deleter()(ptr_);
  }

  unique_ptr& operator=(rv<unique_ptr>& other)
  {
    reset(other.release());
    return *this;
  }

  void reset(T* p = 0) // never throws
  {
    Deleter()(ptr_);
    ptr_ = p;
  }

  T* release()
  {
    T* p = ptr_;
    ptr_ = 0;
    return p;
  }

  T& operator*() const // never throws
  {
    return *get();
  }

  T* operator->() const // never throws
  {
    return get();
  }

  T* get() const // never throws
  {
    return ptr_;
  }

  operator bool() const // never throws
  {
    return get() != 0;
  }

  void swap(unique_ptr& b) // never throws
  {
    std::swap(ptr_, b.ptr_);
  }

  bool operator<(const unique_ptr& other) const
  {
    return ptr_ < other.ptr_;
  }

private:
  unique_ptr(const unique_ptr&);
  unique_ptr& operator=(const unique_ptr&);

  T* ptr_;
};

template <typename T>
typename T::rv_reference move(T& p)
{
  return static_cast<typename T::rv_reference>(p);
}


template <typename T, typename Deleter>
void swap(unique_ptr<T, Deleter>& a, unique_ptr<T, Deleter>& b) // never throws
{
  return a.swap(b);
}

template <typename T>
class container_supported_unique_ptr
{
public:

  container_supported_unique_ptr()
    : ptr_(0)
  {}

  explicit container_supported_unique_ptr(T* p)
    : ptr_(p)
  {
  }

  template <typename U>
  container_supported_unique_ptr(unique_ptr<U> p)
    : ptr_(p.release())
  {
  }

  template <typename U>
  container_supported_unique_ptr(const container_supported_unique_ptr<U>& other)
    : ptr_(other.get())
  {
    this->bump_up();
  }

  container_supported_unique_ptr(const container_supported_unique_ptr& b)
    : ptr_(b.ptr_)
  {
    this->bump_up();
  }

  ~container_supported_unique_ptr()
  {
    this->bump_down();
  }

  template <typename U>
  void reset(U* p)
  {
    container_supported_unique_ptr tmp(p);
    swap(tmp);
  }

  void reset(T* p=0)
  {
    container_supported_unique_ptr tmp(p);
    swap(tmp);
  }

  container_supported_unique_ptr& operator=(const container_supported_unique_ptr& b)
  {
    container_supported_unique_ptr tmp(b);
    swap(tmp);
    return *this;
  }

  template <class U>
  container_supported_unique_ptr& operator=(const container_supported_unique_ptr<U>& b)
  {
    container_supported_unique_ptr<T> tmp(b);
    swap(tmp);
    return *this;
  }

  template <typename U>
  container_supported_unique_ptr& operator=(unique_ptr<U> b)
  {
    container_supported_unique_ptr<T> tmp(b.release());
    swap(tmp);
    return *this;
  }

  void swap(container_supported_unique_ptr& rhs)
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

  T* get() const
  {
    return this->ptr_;
  }

  T* release()
  {
    T* retval = this->ptr_;
    this->ptr_ = 0;
    return retval;
  }

  operator bool() const
  {
    return get() != 0;
  }

  bool operator==(const container_supported_unique_ptr& rhs) const
  {
    return get() == rhs.get();
  }

  bool operator!=(const container_supported_unique_ptr& rhs) const
  {
    return get() != rhs.get();
  }

  bool operator < (const container_supported_unique_ptr& rhs) const
  {
    return get() < rhs.get();
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
void swap(container_supported_unique_ptr<T>& lhs, container_supported_unique_ptr<T>& rhs)
{
  lhs.swap(rhs);
}

template <typename T>
class EnableContainerSupportedUniquePtr {
protected:
  EnableContainerSupportedUniquePtr()
    : ref_count_(1)
  {
  }
private:
  template <typename U>
  friend class container_supported_unique_ptr;

  template <typename U>
  friend typename unique_ptr<U>::rv_reference move(container_supported_unique_ptr<U>& ptr);

  void _add_ref() {
    ++this->ref_count_;
  }

  void _remove_ref(){
    const long new_count = --this->ref_count_;

    if (new_count == 0) {
      delete static_cast<T*>(this);
    }
  }
  long ref_count() const { return ref_count_.value(); }
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> ref_count_;
};

template <typename T>
typename unique_ptr<T>::rv_reference move(container_supported_unique_ptr<T>& ptr)
{
  OPENDDS_ASSERT(ptr->ref_count() == 1);
  return reinterpret_cast<typename unique_ptr<T>::rv_reference>(ptr);
}

#endif
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif /* end of include guard: UNIQUE_PTR_H_18C6F30C */
