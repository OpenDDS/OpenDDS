#ifndef OPENDDS_DCPS_ATOMIC_BOOL_H
#define OPENDDS_DCPS_ATOMIC_BOOL_H

#include "SafeBool_T.h"

#include <dds/Versioned_Namespace.h>

#include <ace/config.h>
#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class AtomicBool : public SafeBool_T<AtomicBool> {
public:
  AtomicBool(bool value)
  : impl_(value)
  {
  }

  bool boolean_test() const
  {
    return impl_
#ifndef ACE_HAS_CPP11
      .value()
#endif
      ;
  }

  AtomicBool& operator=(bool value)
  {
    impl_ = value;
    return *this;
  }

private:
#ifdef ACE_HAS_CPP11
  std::atomic<bool>
#else
  ACE_Atomic_Op<ACE_Thread_Mutex, bool>
#endif
    impl_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
