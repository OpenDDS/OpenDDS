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

/**
 * Barrier provides thread-coordination across an expected number of threads
 *
 * Threads which call Barrier::wait() will all wait until the expected number
 * of threads have called Barrier::wait(), at which point they will all be woken
 * up and the next "phase" of waiting will begin (again, being released when the
 * expected number of waiting threads is reached
 */
class OpenDDS_Dcps_Export Barrier {
public:

  /**
   * Creates a Barrier class of the specified number of expected calls to wait
   * @param expected the number of threads to call wait before being waking
   */
  explicit Barrier(size_t expected);

  virtual ~Barrier();

  /// Waits until the expected number of waiting threads has been reached
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
