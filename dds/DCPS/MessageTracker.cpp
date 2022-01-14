/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "MessageTracker.h"
#include "Service_Participant.h"

#include <ace/Synch.h>
#include <ace/ACE.h>
#include <ace/Guard_T.h>
#include <ace/OS_NS_time.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MessageTracker::MessageTracker(const OPENDDS_STRING& msg_src)
: msg_src_(msg_src)
, dropped_count_(0)
, delivered_count_(0)
, sent_count_(0)
, done_condition_(lock_)
{
}

bool
MessageTracker::pending_messages()
{
  return sent_count_ > delivered_count_ + dropped_count_;
}

void
MessageTracker::message_sent()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  ++sent_count_;
}

void
MessageTracker::message_delivered()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  ++delivered_count_;

  if (!pending_messages()) {
    done_condition_.notify_all();
  }
}

void
MessageTracker::message_dropped()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  ++dropped_count_;

  if (!pending_messages()) {
    done_condition_.notify_all();
  }
}

void MessageTracker::wait_messages_pending(const char* caller)
{
  const TimeDuration pending_timeout(TheServiceParticipant->pending_timeout());
  wait_messages_pending(caller, pending_timeout.is_zero() ?
    MonotonicTimePoint() : MonotonicTimePoint::now() + pending_timeout);
}

void MessageTracker::wait_messages_pending(const char* caller, const MonotonicTimePoint& deadline)
{
  const bool use_deadline = deadline.is_zero();
  ACE_GUARD(ACE_Thread_Mutex, guard, this->lock_);
  const bool report = DCPS_debug_level > 0 && pending_messages();
  if (report) {
    if (use_deadline) {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) MessageTracker::wait_messages_pending ")
                ACE_TEXT("from source=%C will wait until %#T.\n"),
                msg_src_.c_str(), &deadline.value()));
    } else {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) MessageTracker::wait_messages_pending ")
                ACE_TEXT("from source=%C will wait with no timeout.\n"),
                msg_src_.c_str()));
    }
  }
  bool loop = true;
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  while (loop && pending_messages()) {
    switch (done_condition_.wait_until(deadline, thread_status_manager)) {
    case CvStatus_Timeout:
      if (DCPS_debug_level && pending_messages()) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) MessageTracker::wait_messages_pending %T ")
                   ACE_TEXT("(Redmine Issue# 1446) (caller: %C)\n"),
                   ACE_TEXT("Timed out waiting for messages to be transported"),
                   caller));
      }
      loop = false;
      break;

    case CvStatus_NoTimeout:
      break;

    case CvStatus_Error:
      if (DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MessageTracker::wait_messages_pending: "
          "error in wait_until\n"));
      }
      loop = false;
      return;
    }
  }
  if (report) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) MessageTracker::wait_messages_pending %T done\n"));
  }
}

int
MessageTracker::dropped_count()
{
  return dropped_count_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
