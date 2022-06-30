/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ThreadPool.h"

#include <ace/Guard_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ThreadPool::ThreadPool(size_t count, FunPtr fun, void* arg)
 : barrier_(count + 1)
 , fun_(fun)
 , arg_(arg)
 , ids_(count, 0)
{
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    for (size_t i = 0; i < count; ++i) {
      ACE_Thread::spawn(run, this, THR_NEW_LWP | THR_JOINABLE, 0, &(ids_[i]));
    }
  }
  if (count) {
    barrier_.wait();
  }
}

ThreadPool::~ThreadPool()
{
  join_all();
}

ACE_THR_FUNC_RETURN ThreadPool::run(void* arg)
{
  ThreadPool& pool = *(static_cast<ThreadPool*>(arg));
  {
    ACE_Guard<ACE_Thread_Mutex> guard(pool.mutex_);
    pool.id_set_.insert(ACE_Thread::self());
  }
  pool.barrier_.wait();
  (*pool.fun_)(pool.arg_);
  return 0;
}

bool ThreadPool::contains(ACE_thread_t id) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return id_set_.count(id);
}

void ThreadPool::join_all()
{
  OPENDDS_VECTOR(ACE_hthread_t) ids;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ids = ids_;
  }

  for (size_t i = 0; i < ids.size(); ++i) {
    ACE_Thread::join(ids[i], 0);
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
