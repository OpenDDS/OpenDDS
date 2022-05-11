/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ThreadedRtpsSendQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

ThreadedRtpsSendQueue::ThreadedRtpsSendQueue()
: has_data_to_send_(false)
{
}

bool ThreadedRtpsSendQueue::enqueue(const MetaSubmessageVec& vec)
{
  bool result = false;
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  ThreadQueueMap::iterator pos = thread_queue_map_.find(ACE_Thread::self());
  RtpsSendQueue* qptr = 0;
  if (pos == thread_queue_map_.end() || pos->second.enabled() == false) {
    qptr = &primary_queue_;
    result = vec.size() > 0;
    has_data_to_send_ |= result;
  } else {
    qptr = &pos->second;
  }
  for (MetaSubmessageVec::const_iterator it = vec.begin(); it != vec.end(); ++it) {
    qptr->push_back(*it);
  }
  return result;
}

void ThreadedRtpsSendQueue::enable_thread_queue()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  thread_queue_map_[ACE_Thread::self()].enabled(true);
}

bool ThreadedRtpsSendQueue::disable_thread_queue()
{
  bool result = false;
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  ThreadQueueMap::iterator pos = thread_queue_map_.find(ACE_Thread::self());
  if (pos != thread_queue_map_.end()) {
    result = primary_queue_.merge(pos->second);
    has_data_to_send_ |= result;
    pos->second.enabled(false);
  }
  return result;
}

void ThreadedRtpsSendQueue::condense_and_swap(MetaSubmessageVec& vec)
{
  vec.clear();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!has_data_to_send_) {
    return;
  }
  primary_queue_.condense_and_swap(vec);
  has_data_to_send_ = false;
}

void ThreadedRtpsSendQueue::purge_remote(const RepoId& id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (ThreadQueueMap::iterator it = thread_queue_map_.begin(); it != thread_queue_map_.end(); ++it) {
    it->second.purge_remote(id);
  }
  primary_queue_.purge_remote(id);
}

void ThreadedRtpsSendQueue::purge_local(const RepoId& id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (ThreadQueueMap::iterator it = thread_queue_map_.begin(); it != thread_queue_map_.end(); ++it) {
    it->second.purge_local(id);
  }
  primary_queue_.purge_local(id);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
