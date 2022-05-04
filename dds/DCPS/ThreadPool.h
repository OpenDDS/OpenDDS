/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADPOOL_H
#define OPENDDS_DCPS_THREADPOOL_H

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

private:

  void join_all();

  Barrier barrier_;
  FunPtr fun_;
  void* arg_;
  OPENDDS_VECTOR(ACE_hthread_t) ids_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_THREADPOOL_H
