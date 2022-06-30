/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREAD_POOL_H
#define OPENDDS_DCPS_THREAD_POOL_H

#include "dcps_export.h"

#include "PoolAllocator.h"

#include <ace/Barrier.h>
#include <ace/Thread.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * ThreadPool is a light-weight utility class for starting a group of threads
 *
 * ThreadPool creates several threads at construction and attempts to join them
 * at destruction. Users of ThreadPool are responsible for making sure the
 * running threads are in a joinable state before the destruction of ThreadPool
 */
class OpenDDS_Dcps_Export ThreadPool
{
public:

  /// A typedef for the starting point of the ThreadPool
  typedef ACE_THR_FUNC_RETURN(*FunPtr)(void*);

  /**
   * Creates a ThreadPool with the specifed size, starting point, and argument
   * @param count number of threads
   * @param fun starting point for the threads of the ThreadPool
   * @param arg an optional argument to pass to the starting function
   */
  ThreadPool(size_t count, FunPtr fun, void* arg = 0);
  virtual ~ThreadPool();

  /// A static helper function used to redirect to requested thread start point
  static ACE_THR_FUNC_RETURN run(void* arg);

  /**
   * Check if a specific thread id belongs to this ThreadPool
   * @param id thread id to check
   * @returns true if the ThreadPool contains thread with specified id, false otherwise
   */
  bool contains(ACE_thread_t id) const;

private:

  void join_all();

  ACE_Barrier barrier_;
  FunPtr fun_;
  void* arg_;
  mutable ACE_Thread_Mutex mutex_;
  OPENDDS_VECTOR(ACE_hthread_t) ids_;
  OPENDDS_SET(ACE_thread_t) id_set_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_THREAD_POOL_H
