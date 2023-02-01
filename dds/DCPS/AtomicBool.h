#ifndef OPENDDS_DCPS_ATOMIC_BOOL_H
#define OPENDDS_DCPS_ATOMIC_BOOL_H

#include "Atomic.h"
#include "SafeBool_T.h"

#include <dds/Versioned_Namespace.h>

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
    return impl_;
  }

  AtomicBool& operator=(bool value)
  {
    impl_ = value;
    return *this;
  }

private:
  Atomic<bool> impl_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
