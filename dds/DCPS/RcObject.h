#ifndef OPENDDS_DCPS_RCOBJECT_H
#define OPENDDS_DCPS_RCOBJECT_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


#include "dds/Versioned_Namespace.h"

#include "dcps_export.h"
#include "Atomic.h"
#include "PoolAllocationBase.h"
#include "RcHandle_T.h"

#include <ace/Guard_T.h>
#include <ace/Synch_Traits.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class RcObject;

  class OpenDDS_Dcps_Export WeakObject : public PoolAllocationBase
  {
  public:

    WeakObject(RcObject* ptr)
      : ptr_(ptr)
      , ref_count_(1)
      , expired_(false)
    {
    }

    void _add_ref()
    {
      ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
      ++ref_count_;
    }

    void _remove_ref()
    {
      ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
      const long new_count = --ref_count_;
      if (new_count == 0) {
        guard.release();
        delete this;
      }
    }

    RcObject* lock();
    bool check_expire(Atomic<long>& count);

  private:
    ACE_SYNCH_MUTEX mx_;
    RcObject* const ptr_;
    long ref_count_;
    bool expired_;
  };

  class OpenDDS_Dcps_Export RcObject : public PoolAllocationBase {
  public:

    virtual ~RcObject()
    {
      weak_object_->_remove_ref();
    }

    virtual void _add_ref()
    {
      ++ref_count_;
    }

    virtual void _remove_ref()
    {
      if (weak_object_->check_expire(ref_count_)) {
        delete this;
      }
    }

    /// This accessor is purely for debugging purposes
    long ref_count() const
    {
      return ref_count_;
    }

    WeakObject* _get_weak_object() const
    {
      weak_object_->_add_ref();
      return weak_object_;
    }

  protected:
    RcObject()
      : ref_count_(1)
      , weak_object_(new WeakObject(this))
    {}

  private:
    Atomic<long> ref_count_;
    WeakObject* weak_object_;

    RcObject(const RcObject&);
    RcObject& operator=(const RcObject&);
  };

  inline RcObject* WeakObject::lock()
  {
    ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
    if (ptr_) {
      ptr_->_add_ref();
    }
    return ptr_;
  }

  inline bool WeakObject::check_expire(Atomic<long>& count)
  {
    ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
    const long new_count = --count;
    if (new_count == 0 && ptr_) {
      ptr_ = 0;
      return true;
    }
    return false;
  }

  template <typename T>
  class WeakRcHandle
  {
  public:
    WeakRcHandle()
      : weak_object_(0)
      , cached_(0)
    {
    }

    WeakRcHandle(const T& obj)
      : weak_object_(obj._get_weak_object())
      , cached_(const_cast<T*>(&obj))
    {
    }

    WeakRcHandle(const RcHandle<T>& rch)
      : weak_object_(rch.in() ? rch.in()->_get_weak_object() : 0)
      , cached_(rch.in())
    {
    }

    WeakRcHandle(const WeakRcHandle& other)
      : weak_object_(other.weak_object_)
      , cached_(other.cached_)
    {
      if (weak_object_) {
        weak_object_->_add_ref();
      }
    }

    ~WeakRcHandle()
    {
      if (weak_object_) {
        weak_object_->_remove_ref();
      }
    }

    WeakRcHandle& operator=(const WeakRcHandle& other)
    {
       WeakRcHandle tmp(other);
       std::swap(weak_object_, tmp.weak_object_);
       std::swap(cached_, tmp.cached_);
       return *this;
    }

    WeakRcHandle& operator=(const RcHandle<T>& other)
    {
       WeakRcHandle tmp(other);
       std::swap(weak_object_, tmp.weak_object_);
       std::swap(cached_, tmp.cached_);
       return *this;
    }

    WeakRcHandle& operator=(const T& obj)
    {
      WeakRcHandle tmp(obj);
      std::swap(weak_object_, tmp.weak_object_);
      std::swap(cached_, tmp.cached_);
      return *this;
    }

    RcHandle<T> lock() const
    {
      if (weak_object_ && weak_object_->lock()) {
        return RcHandle<T>(cached_, keep_count());
      }
      return RcHandle<T>();
    }

    bool operator==(const WeakRcHandle& rhs) const
    {
      return weak_object_ == rhs.weak_object_;
    }

    bool operator!=(const WeakRcHandle& rhs) const
    {
      return weak_object_ != rhs.weak_object_;
    }

    bool operator<(const WeakRcHandle& rhs) const
    {
      return weak_object_ < rhs.weak_object_;
    }

    operator bool() const
    {
      return weak_object_;
    }

    void reset()
    {
      if (weak_object_) {
        weak_object_->_remove_ref();
        weak_object_ = 0;
      }
      cached_ = 0;
    }

  private:

    WeakRcHandle(WeakObject* obj)
      : weak_object_(obj)
      , cached_(dynamic_cast<T*>(obj))
    {
    }

    WeakObject* weak_object_;
    T* cached_;
  };

} // DCPS
} // OPENDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: OPENDDS_DCPS_RCOBJECT_H */
