/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREAD_POOL_H
#define OPENDDS_DCPS_THREAD_POOL_H

#include "dcps_export.h"

#include "Barrier.h"
#include "PoolAllocator.h"

#include <ace/Thread.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

class OpenDDS_Dcps_Export ThreadPool
{
public:

  typedef ACE_THR_FUNC_RETURN(*FunPtr)(void*);

  ThreadPool(size_t count, FunPtr fun, void* arg = 0);
  virtual ~ThreadPool();

  static ACE_THR_FUNC_RETURN run(void* arg);

  bool contains(ACE_thread_t) const;

private:

  void join_all();

  Barrier barrier_;
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
