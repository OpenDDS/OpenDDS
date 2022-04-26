/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/ThreadPool.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

ThreadPool::ThreadPool(size_t count, FunPtr fun, void* arg)
 : ids_(count, 0)
{
  for (size_t i = 0; i < count; ++i) {
    ACE_Thread::spawn(fun, arg, THR_NEW_LWP | THR_JOINABLE, &(ids_[i]));
  }
}

ThreadPool::~ThreadPool()
{
  join_all();
}

void ThreadPool::join_all()
{
  for (size_t i = 0; i < ids_.size(); ++i) {
    ACE_Thread::join(ids_[i], 0);
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
