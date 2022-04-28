/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADPOOL_H
#define OPENDDS_DCPS_THREADPOOL_H

#include "dcps_export.h"
#include "ConditionVariable.h"
#include "PoolAllocator.h"
#include "ThreadStatusManager.h"

#include <ace/Thread.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

class OpenDDS_Dcps_Export ThreadPool
{
public:

  class Barrier
  {
  public:

    Barrier(size_t expected) : mutex_(), tsm_(), cv_(mutex_), expected_(expected), count_(0), waiting_(0), running_(true)
    {
    }

    virtual ~Barrier()
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      running_ = false;
      cv_.notify_all();
      while (waiting_) {
        cv_.wait(tsm_);
      }
    }

    void wait()
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      const size_t next_generation = (count_ / expected_) + 1;
      ++count_;
      cv_.notify_all();
      if (expected_) {
        ++waiting_;
        size_t current_generation = count_ / expected_;
        while (running_ && current_generation != next_generation) {
          cv_.wait(tsm_);
          current_generation = count_ / expected_;
        }
        --waiting_;
        if (!running_) {
          cv_.notify_one();
        }
      }
    }

  private:
    ACE_Thread_Mutex mutex_;
    ThreadStatusManager tsm_;
    ConditionVariable<ACE_Thread_Mutex> cv_;
    const size_t expected_;
    size_t count_;
    size_t waiting_;
    bool running_;
  };

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
