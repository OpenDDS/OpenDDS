#ifndef OPENDDS_DCPS_ATOMIC_H
#define OPENDDS_DCPS_ATOMIC_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef ACE_HAS_CPP11
template <typename T>
using Atomic = std::atomic<T>;
#else
template <typename T>
class Atomic : public ACE_Atomic_Op<ACE_SYNCH_MUTEX, T>
{
public:
  typedef ACE_Atomic_Op<ACE_SYNCH_MUTEX, T> Base;
  Atomic() : Base() {}
  explicit Atomic(T desired) : Base(desired) {}
  inline operator T() const { return Base::value(); }
};
#endif

} // DCPS
} // OPENDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* end of include guard: OPENDDS_DCPS_ATOMIC_H */

