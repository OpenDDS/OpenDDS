#ifndef WEAK_RCHANDLE_H_E92AD5BB
#define WEAK_RCHANDLE_H_E92AD5BB

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


#include "dds/Versioned_Namespace.h"
#include "ace/Atomic_Op.h"
#include "ace/Synch_Traits.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  template <typename T>
  class WeakObject : public PoolAllocationBase
  {
  public:
    WeakObject(T* ptr)
      : ref_count_(1)
      , ptr_(ptr)
      , expired_(false)
    {
    }

    void _add_ref() {
      ++this->ref_count_;
    }

    T* lock(){
      ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
      if (! expired_) {
        ptr_->_add_ref();
        return ptr_;
      }
      return 0;
    }

    bool set_expire(){
      ACE_Guard<ACE_SYNCH_MUTEX> guard(mx_);
      if (!expired_ && ptr_->ref_count() == 0) {
        expired_ = true;
      }
      return expired_;
    }

    void _remove_ref(){
      const long new_count = --this->ref_count_;

      if (new_count == 0) {
        delete this;
      }
    }
  private:
    ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> ref_count_;
    ACE_SYNCH_MUTEX mx_;
    T* const ptr_;
    bool expired_;
  };

  template <typename T>
  class WeakRcHandle
  {
  public:
    WeakRcHandle()
      : weak_object_(0)
    {
    }

    WeakRcHandle(const RcHandle<T>& rch)
      : weak_object_(rch.in() ? rch.get()->_get_weak_object() : 0) {
    }

    WeakRcHandle(const WeakRcHandle& other)
    : weak_object_(other.weak_object_){
      if (weak_object_)
        weak_object_->_add_ref();
    }

    ~WeakRcHandle(){
      if (weak_object_)
        weak_object_->_remove_ref();
    }

    WeakRcHandle& operator = (const WeakRcHandle& other) {
       WeakRcHandle tmp(other);
       std::swap(weak_object_, tmp.weak_objct_);
       return *this;
    }

    WeakRcHandle& operator = (const RcHandle<T>& other) {
       WeakRcHandle tmp(other);
       std::swap(weak_object_, tmp.weak_objct_);
       return *this;
    }

    RcHandle<T> lock() const {
      if (weak_object_){
        return RcHandle<T>(weak_object_->lock(), keep_count());
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

    bool operator < (const WeakRcHandle& rhs) const
    {
      return weak_object_ < rhs.weak_object_;
    }

  private:
    template <typename Base, typename Derived>
    friend class EnableWeakRcHandle;

    WeakRcHandle(WeakObject<T>* obj)
      : weak_object_(obj)
    {
    }

    WeakObject<T>* weak_object_;
  };

  /// Base is the base class where _add_ref() and _remove_ref() needed to be overrieded
  /// Derived is the most derived class
  template <typename Base, typename Derived>
  class EnableWeakRcHandle : public Base {
  public:

    virtual ~EnableWeakRcHandle(){
      weak_object_->_remove_ref();
    }

    virtual void _add_ref() {
      ++this->ref_count_;
    }

    virtual void _remove_ref() {
      const long new_count = --this->ref_count_;
      if (new_count == 0 && weak_object_->set_expire()) {
        delete this;
      }
    }

    WeakRcHandle<Derived> weak_rchandle_from_this() const {
      return WeakRcHandle<Derived>(_get_weak_object());
    }

    /// This accessor is purely for debugging purposes
    long ref_count() const {
      return this->ref_count_.value();
    }

  protected:

    EnableWeakRcHandle()
      : ref_count_(1)
      , weak_object_( new WeakObject<Derived>(static_cast<Derived*>(this)) )
    {}


  private:
    template <typename U>
    friend class WeakRcHandle;

    WeakObject<Derived>*
    _get_weak_object() const {
      weak_object_->_add_ref();
      return weak_object_;
    }

    ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> ref_count_;
    WeakObject<Derived>*  weak_object_;

    EnableWeakRcHandle(const EnableWeakRcHandle&);
    EnableWeakRcHandle& operator=(const EnableWeakRcHandle&);
  };

}// DCPS
}// OPENDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: WEAK_RCHANDLE_H_E92AD5BB */
