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
, done_condition_(lock_, ConditionAttributesMonotonic())
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

  if (!pending_messages())
    done_condition_.broadcast();
}

void
MessageTracker::message_dropped()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  ++dropped_count_;

  if (!pending_messages())
    done_condition_.broadcast();
}

void MessageTracker::wait_messages_pending(const char* caller)
{
  const TimeDuration pending_timeout(TheServiceParticipant->pending_timeout());
  wait_messages_pending(caller, pending_timeout.is_zero() ?
    MonotonicTimePoint() : MonotonicTimePoint::now() + pending_timeout);
}

void MessageTracker::wait_messages_pending(const char* caller, const MonotonicTimePoint& deadline)
{
  const ACE_Time_Value_T<MonotonicClock>* deadline_ptr = deadline.is_zero() ? 0 : &deadline.value();
  ACE_GUARD(ACE_Thread_Mutex, guard, this->lock_);
  const bool report = DCPS_debug_level > 0 && pending_messages();
  if (report) {
    if (deadline_ptr) {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) MessageTracker::wait_messages_pending ")
                ACE_TEXT("from source=%C will wait until %#T.\n"),
                msg_src_.c_str(), deadline_ptr));
    } else {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t) MessageTracker::wait_messages_pending ")
                ACE_TEXT("from source=%C will wait with no timeout.\n")));
    }
  }
  while (true) {
    if (!pending_messages())
      break;

    if (done_condition_.wait(deadline_ptr) == -1 && pending_messages()) {
      if (DCPS_debug_level) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) %T MessageTracker::")
                   ACE_TEXT("wait_messages_pending (Redmine Issue# 1446) %p (caller: %C)\n"),
                   ACE_TEXT("Timed out waiting for messages to be transported"),
                   caller));
      }
      break;
    }
  }
  if (report) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) MessageTracker::wait_messages_pending done\n"));
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
