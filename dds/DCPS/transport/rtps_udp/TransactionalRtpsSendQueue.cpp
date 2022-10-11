/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransactionalRtpsSendQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransactionalRtpsSendQueue::TransactionalRtpsSendQueue(ThreadStatusManager& thread_status_manager)
  : condition_variable_(mutex_)
  , active_transaction_count_(0)
  , thread_status_manager_(thread_status_manager)
{
}

bool TransactionalRtpsSendQueue::enqueue(const MetaSubmessage& ms)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  const bool empty_before = queue_.empty();
  queue_.push_back(ms);
  return empty_before;
}

bool TransactionalRtpsSendQueue::enqueue(const MetaSubmessageVec& vec)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  const bool empty_before = queue_.empty();
  for (MetaSubmessageVec::const_iterator it = vec.begin(), limit = vec.end(); it != limit; ++it) {
    queue_.push_back(*it);
  }
  return empty_before && !queue_.empty();
}

void TransactionalRtpsSendQueue::begin_transaction()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  ++active_transaction_count_;
  condition_variable_.notify_one();
}

bool TransactionalRtpsSendQueue::end_transaction()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  --active_transaction_count_;
  condition_variable_.notify_one();
  return !queue_.empty();
}

size_t TransactionalRtpsSendQueue::active_transaction_count() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return active_transaction_count_;
}

void TransactionalRtpsSendQueue::swap(MetaSubmessageVec& vec)
{
  vec.clear();

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  while (active_transaction_count_ != 0) {
    condition_variable_.wait(thread_status_manager_);
  }

  queue_.swap(vec);
}

void TransactionalRtpsSendQueue::purge(const RepoId& local, const RepoId& remote)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (MetaSubmessageVec::iterator pos = queue_.begin(), limit = queue_.end(); pos != limit; ++pos) {
    if (pos->src_guid_ == local && pos->dst_guid_ == remote) {
      pos->ignore_ = true;
    }
  }
}

void TransactionalRtpsSendQueue::purge_remote(const RepoId& id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (MetaSubmessageVec::iterator pos = queue_.begin(), limit = queue_.end(); pos != limit; ++pos) {
    if (pos->dst_guid_ == id) {
      pos->ignore_ = true;
    }
  }
}

void TransactionalRtpsSendQueue::purge_local(const RepoId& id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (MetaSubmessageVec::iterator pos = queue_.begin(), limit = queue_.end(); pos != limit; ++pos) {
    if (pos->src_guid_ == id) {
      pos->ignore_ = true;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
