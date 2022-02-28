#ifndef OPENDDS_DCPS_RCOBJECT_H
#define OPENDDS_DCPS_RCOBJECT_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


#include "dds/Versioned_Namespace.h"

#include "dcps_export.h"
#include "PoolAllocationBase.h"
#include "RcHandle_T.h"

#include <ace/Guard_T.h>
#include <ace/Synch_Traits.h>

#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  class RcObject;

  class OpenDDS_Dcps_Export WeakObject : public PoolAllocationBase
  {
  public:
    WeakObject(RcObject* ptr)
      : ref_count_(1)
      , ptr_(ptr)
      , expired_(false)
    {
    }

    void _add_ref()
    {
      ++ref_count_;
    }

    void _remove_ref()
    {
      const long new_count = --ref_count_;

      if (new_count == 0) {
        delete this;
      }
    }

    RcObject* lock();
    bool set_expire();

  private:
#ifdef ACE_HAS_CPP11
    std::atomic<long> ref_count_;
#else
    ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> ref_count_;
#endif
    ACE_SYNCH_MUTEX mx_;
    RcObject* const ptr_;
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
      const long new_count = --ref_count_;
      if (new_count == 0 && weak_object_->set_expire()) {
        delete this;
      }
    }

    /// This accessor is purely for debugging purposes
    long ref_count() const
    {
#ifdef ACE_HAS_CPP11
      return ref_count_;
#else
      return ref_count_.value();
#endif
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
#ifdef ACE_HAS_CPP11
    std::atomic<long> ref_count_;
#else
    ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> ref_count_;
#endif
    WeakObject*  weak_object_;

    RcObject(const RcObject&);
    RcObject& operator=(const RcObject&);
  };

  inline RcObject* WeakObject::lock()
  {
    ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
    if (!expired_) {
      ptr_->_add_ref();
      return ptr_;
    }
    return 0;
  }

  inline bool WeakObject::set_expire()
  {
    ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
    if (!expired_ && ptr_->ref_count() == 0) {
      expired_ = true;
    }
    return expired_;
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
