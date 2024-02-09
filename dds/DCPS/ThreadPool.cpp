/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Definitions.h"
#include "ThreadPool.h"

#include "Definitions.h"

#include <ace/Guard_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ThreadPool::ThreadPool(size_t count, FunPtr fun, void* arg)
 : fun_(fun)
 , arg_(arg)
 , mutex_()
 , cv_(mutex_)
 , active_threads_(0)
#ifdef OPENDDS_NO_THREAD_JOIN
 , finished_threads_(0)
#endif
 , ids_(count)
{
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    for (size_t i = 0; i < count; ++i) {
      ACE_Thread::spawn(run, this, THR_NEW_LWP | THR_JOINABLE, 0, &(ids_[i]));
    }
  }
  if (count) {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    while (active_threads_ != count) {
      cv_.wait(tsm_);
    }
  }
}

ThreadPool::~ThreadPool()
{
  join_all();
}

ACE_THR_FUNC_RETURN ThreadPool::run(void* arg)
{
  ThreadPool& pool = *static_cast<ThreadPool*>(arg);
  {
    ACE_Guard<ACE_Thread_Mutex> guard(pool.mutex_);
    pool.id_set_.insert(ACE_Thread::self());
    ++pool.active_threads_;
    pool.cv_.notify_all();
    while (pool.active_threads_ != pool.ids_.size()) {
      pool.cv_.wait(pool.tsm_);
    }
  }
  (*pool.fun_)(pool.arg_);
#ifdef OPENDDS_NO_THREAD_JOIN
  ACE_Guard<ACE_Thread_Mutex> guard(pool.mutex_);
  ++pool.finished_threads_;
  pool.cv_.notify_one();
#endif
  return 0;
}

bool ThreadPool::contains(ACE_thread_t id) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return id_set_.count(id);
}

void ThreadPool::join_all()
{
#ifdef OPENDDS_NO_THREAD_JOIN
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  while (finished_threads_ != ids_.size()) {
    cv_.wait(tsm_);
  }
#else
  OPENDDS_VECTOR(ACE_hthread_t) ids;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ids = ids_;
  }

  for (size_t i = 0; i < ids.size(); ++i) {
    const int result = ACE_Thread::join(ids[i], 0);
    ACE_UNUSED_ARG(result);
    OPENDDS_ASSERT(result == 0);
  }
#endif
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
