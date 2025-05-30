#ifndef OPENDDS_DCPS_MESSAGE_BLOCK_PTR_H
#define OPENDDS_DCPS_MESSAGE_BLOCK_PTR_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dcps_export.h"
#include "unique_ptr.h"

#include <ace/Guard_T.h>
#include <ace/Lock_Adapter_T.h>
#include <ace/Message_Block.h>
#include <ace/Thread_Mutex.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct Message_Block_Deleter {
  void operator()(ACE_Message_Block* ptr) const
  {
    // In order to avoid locking order issues for message blocks with different locking strategies,
    // it is safer to unlink elements in the chain before releasing them individually
    while (ptr) {
      ACE_Message_Block* cont = ptr->cont();
      ptr->cont(0);
      ACE_Message_Block::release(ptr);
      ptr = cont;
    }
  }
};

typedef unique_ptr<ACE_Message_Block, Message_Block_Deleter> Message_Block_Ptr;

/**
 * Copyable smart pointer that uses ACE_Message_Block / ACE_Data_Block reference counting
 *
 * Each Message_Block_Shared_Ptr has its own distinct ACE_Message_Block and the set of
 * Message_Block_Shared_Ptr objects that were the result of copying/assigning all refer to
 * the same ACE_Data_Block.  Two Message_Block_Shared_Ptr objects that refer to the same
 * ACE_Data_Bock may have different rd_ptr() and cont() values, for example.
 */
class OpenDDS_Dcps_Export Message_Block_Shared_Ptr {
public:
  Message_Block_Shared_Ptr() {}

  explicit Message_Block_Shared_Ptr(ACE_Message_Block* ptr)
    : msg_(ptr)
  {}

  Message_Block_Shared_Ptr(const Message_Block_Shared_Ptr& other)
    : msg_(ACE_Message_Block::duplicate(other.msg_.get()))
  {}

  Message_Block_Shared_Ptr& operator=(const Message_Block_Shared_Ptr& other)
  {
    Message_Block_Shared_Ptr cpy(other);
    swap(*this, cpy);
    return *this;
  }

#ifdef ACE_HAS_CPP11
  Message_Block_Shared_Ptr(Message_Block_Shared_Ptr&&) = default;
  Message_Block_Shared_Ptr& operator=(Message_Block_Shared_Ptr&&) = default;
#endif

  operator bool() const { return msg_.get(); }

  ACE_Message_Block& operator*() const { return *msg_; }

  ACE_Message_Block* operator->() const { return msg_.get(); }

  ACE_Message_Block* get() const { return msg_.get(); }

  friend void swap(Message_Block_Shared_Ptr& a, Message_Block_Shared_Ptr& b)
  {
    using std::swap;
    swap(a.msg_, b.msg_);
  }

private:
  Message_Block_Ptr msg_;
};

#ifdef ACE_HAS_CPP11
/**
 * Copyable smart pointer that uses ACE_Message_Block / ACE_Data_Block reference counting
 * and supports setting the locking_strategy() of the message block to make the shared
 * reference count thread-safe
 *
 * This is logically an extension of Message_Block_Shared_Ptr (above).
 * Locking is not enabled by default but can be enabled by the second constructor parameter.
 * Lockable_Message_Block_Ptr includes lifetime management for the locking strategy object,
 * which ACE doesn't provide.
 */
class OpenDDS_Dcps_Export Lockable_Message_Block_Ptr {
public:
  using Lock_Shared_Ptr = std::shared_ptr<ACE_Lock_Adapter<ACE_Thread_Mutex>>;
  enum class Lock_Policy { No_Lock, Use_Lock };

  explicit Lockable_Message_Block_Ptr(ACE_Message_Block* ptr = nullptr,
                                      Lock_Policy policy = Lock_Policy::No_Lock)
    : lock_(policy == Lock_Policy::Use_Lock
            ? std::make_shared<Lock_Shared_Ptr::element_type>()
            : Lock_Shared_Ptr{})
    , msgblock_(ptr)
  {
    if (lock_ && ptr) {
      ptr->locking_strategy(lock_.get());
    }
  }

  /// Copy from a Message_Block_Shared_Ptr.  The resulting object can't be locked.
  Lockable_Message_Block_Ptr(const Message_Block_Shared_Ptr& mbsp)
    : msgblock_(mbsp)
  {}

  Lockable_Message_Block_Ptr(const Lockable_Message_Block_Ptr&) = default;

  Lockable_Message_Block_Ptr(Lockable_Message_Block_Ptr&&) = default;

  Lockable_Message_Block_Ptr& operator=(const Lockable_Message_Block_Ptr& other)
  {
    Lockable_Message_Block_Ptr cpy(other);
    swap(*this, cpy);
    return *this;
  }

  Lockable_Message_Block_Ptr& operator=(Lockable_Message_Block_Ptr&& other)
  {
    if (&other != this) {
      // msgblock_ first so that this->msgblock_ can use this->lock_ in its own operator=
      msgblock_ = exchange(other.msgblock_, Message_Block_Shared_Ptr{});
      lock_ = exchange(other.lock_, Lock_Shared_Ptr{});
    }
    return *this;
  }

  Lock_Policy lock_policy() const { return lock_ ? Lock_Policy::Use_Lock : Lock_Policy::No_Lock; }

  explicit operator bool() const { return get(); }

  ACE_Message_Block& operator*() const { return *get(); }

  ACE_Message_Block* operator->() const { return get(); }

  ACE_Message_Block* get() const { return msgblock_.get(); }

  /**
   * Set the continuation of this message block to the 'cont' message block.
   *
   * The locking strategy of 'cont' is moved to this object.
   * This is needed because the ACE_Message_Block::cont_ member is not a smart pointer
   * so it can't collaborate on lifetime management for the locking strategy.
   */
  void lockable_cont(const Lockable_Message_Block_Ptr& cont)
  {
    lock_ = cont.lock_;
    Guard g(lock_);
    msgblock_->locking_strategy(cont.msgblock_->locking_strategy(nullptr));
    msgblock_->cont(cont->duplicate());
  }

  friend void swap(Lockable_Message_Block_Ptr& a, Lockable_Message_Block_Ptr& b)
  {
    using std::swap;
    swap(a.lock_, b.lock_);
    swap(a.msgblock_, b.msgblock_);
  }

private:
  Lock_Shared_Ptr lock_;
  Message_Block_Shared_Ptr msgblock_;

  struct Guard : ACE_Guard<Lock_Shared_Ptr::element_type> {
    explicit Guard(const Lock_Shared_Ptr& lock)
      : ACE_Guard(lock.get())
    {
      if (lock) {
        acquire();
      } else {
        owner_ = -1; // do not attempt unlock in ~ACE_Guard()
      }
    }
  };

  // In C++14 this is std::exchange
  template <typename T, typename U = T>
  T exchange(T& from, U&& to)
  {
    const T old = std::move(from);
    from = std::forward<U>(to);
    return old;
  }
};
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
