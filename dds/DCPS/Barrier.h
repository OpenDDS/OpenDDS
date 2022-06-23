/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BARRIER_H
#define OPENDDS_DCPS_BARRIER_H

#include "dcps_export.h"

#include "ConditionVariable.h"
#include "ThreadStatusManager.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export Barrier {
public:

  explicit Barrier(size_t expected);

  virtual ~Barrier();

  void wait();

private:
  ACE_Thread_Mutex mutex_;
  ThreadStatusManager tsm_;
  ConditionVariable<ACE_Thread_Mutex> cv_;
  const size_t expected_;
  size_t count_;
  size_t waiting_;
  bool running_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_BARRIER_H
