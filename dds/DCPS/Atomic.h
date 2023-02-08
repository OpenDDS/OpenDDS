#ifndef OPENDDS_DCPS_ATOMIC_H
#define OPENDDS_DCPS_ATOMIC_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <dds/Versioned_Namespace.h>

#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op.h>
#  include <ace/Thread_Mutex.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef ACE_HAS_CPP11
template <typename T>
using Atomic = std::atomic<T>;
#else
template <typename T>
class Atomic
{
public:
  Atomic() : impl_() {}
  Atomic(T desired) : impl_(desired) {}

  inline T load() const { return impl_.value(); }
  inline void store(T desired) const { impl_ = desired; }
  inline T exchange(const T& desired) { return impl_.exchange(desired); }

  inline operator T() const { return impl_.value(); }
  inline T operator=(const T& desired) { impl_ = desired; return desired; }

  inline T operator++() { return ++impl_; }
  inline T operator++(int) { return impl_++; }

  inline T operator--() { return --impl_; }
  inline T operator--(int) { return impl_--; }

  inline T operator+=(T arg) { return impl_ += arg; }
  inline T operator-=(T arg) { return impl_ -= arg; }

private:
  typedef ACE_Atomic_Op<ACE_Thread_Mutex, T> Base;
  Base impl_;
};
#endif

} // DCPS
} // OPENDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: OPENDDS_DCPS_ATOMIC_H */

