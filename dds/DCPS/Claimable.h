/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CLAIMABLE_H
#define OPENDDS_DCPS_CLAIMABLE_H

#include "Definitions.h"

#include "ace/Thread_Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T> class Claim;

template <typename T>
class Claimable {
public:
  typedef Claim<T> Claim;

  explicit Claimable(const T& data)
    : core_(data)
  {}

private:
  friend Claim;
  T core_;
  mutable ACE_Thread_Mutex mutex_;
};

template <typename T>
class Claim {
public:
  Claim(Claimable<T>& claimable)
    : claimable_(claimable)
  {
    claimable_.mutex_.acquire();
  }

  ~Claim() {
    claimable_.mutex_.release();
  }

  T* operator->() const {
    return &claimable_.core_;
  }

private:
  Claimable<T>& claimable_;
  OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(Claim)
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_CLAIMABLE_H */
