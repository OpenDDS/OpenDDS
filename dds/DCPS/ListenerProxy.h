/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LISTENERPROXY_H
#define OPENDDS_DCPS_LISTENERPROXY_H

#include "ace/Recursive_Thread_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ListenerProxy {
public:
  ListenerProxy()
    : mutex_(0)
  {}

  template <typename T>
  ListenerProxy(ACE_Recursive_Thread_Mutex& mutex,
                T& listener)
    : mutex_(0)
  {
    acquire(mutex, listener);
  }

  ~ListenerProxy() {
    release();
  }

  template <typename T>
  void acquire(ACE_Recursive_Thread_Mutex& mutex,
               T& listener)
  {
    if (mutex_) {
      ACE_ERROR((LM_ERROR, "Attempting to double lock\n"));
      return;
    }

    mutex_ = &mutex;
    mutex_->acquire();

    acquire_i(listener.in());
  }

  void release()
  {
    if (mutex_) {
      mutex_->release();
      mutex_ = 0;
      release_i();
    }
  }

  virtual bool is_nil() const = 0;

private:
  virtual void acquire_i(DDS::Listener_ptr listener) = 0;
  virtual void release_i() = 0;

  ACE_Recursive_Thread_Mutex* mutex_;

  OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(ListenerProxy)
};

template <typename Listener>
class TypedListenerProxy : public ListenerProxy {
public:
  TypedListenerProxy()
    : ListenerProxy()
  {}

  template<typename T>
  TypedListenerProxy(ACE_Recursive_Thread_Mutex& mutex,
                     T& listener)
    : ListenerProxy(mutex, listener)
  {}

  bool is_nil() const
  {
    return CORBA::is_nil(listener_.in());
  }

  typename Listener::_ptr_type in() const
  {
    return listener_.in();
  }

protected:
  typedef typename Listener::_var_type VarType;
  VarType listener_;

private:
  void acquire_i(DDS::Listener_ptr listener)
  {
    listener_ = Listener::_narrow(listener);
  }

  void release_i()
  {
    listener_ = VarType();
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
