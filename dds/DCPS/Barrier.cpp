/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Barrier.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

Barrier::Barrier(size_t expected)
 : mutex_()
 , tsm_()
 , cv_(mutex_)
 , expected_(expected)
 , count_(0)
 , waiting_(0)
 , running_(true)
{
}

Barrier::~Barrier()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  running_ = false;
  cv_.notify_all();
  while (waiting_) {
    cv_.wait(tsm_);
  }
}

void Barrier::wait()
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

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
