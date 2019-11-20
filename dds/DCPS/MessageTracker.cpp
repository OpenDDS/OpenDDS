/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ace/Synch.h"
#include <dds/DCPS/MessageTracker.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>
#include "ace/ACE.h"
#include "ace/Guard_T.h"
#include "ace/OS_NS_time.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

using namespace OpenDDS::DCPS;

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
  if (sent_count_ > delivered_count_ + dropped_count_) {
    return true;
  }
  return false;
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

void
MessageTracker::wait_messages_pending(OPENDDS_STRING& caller_message)
{
  const TimeDuration pending_timeout(TheServiceParticipant->pending_timeout());
  MonotonicTimePoint timeout_at;
  const ACE_Time_Value_T<MonotonicClock>* timeout_ptr = 0;

  if (!pending_timeout.is_zero()) {
    timeout_at = MonotonicTimePoint::now() + pending_timeout;
    timeout_ptr = &timeout_at.value();
  }

  ACE_GUARD(ACE_Thread_Mutex, guard, this->lock_);
  const bool report = DCPS_debug_level > 0 && pending_messages();
  if (report) {
    if (timeout_ptr) {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("%T (%P|%t) MessageTracker::wait_messages_pending ")
                ACE_TEXT("from source=%C will wait until %#T.\n"),
                msg_src_.c_str(), &pending_timeout));
    } else {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("%T (%P|%t) MessageTracker::wait_messages_pending ")
                ACE_TEXT("from source=%C will wait with no timeout.\n")));
    }
  }
  while (true) {
    if (!pending_messages())
      break;

    if (done_condition_.wait(timeout_ptr) == -1 && pending_messages()) {
      if (DCPS_debug_level) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) %T MessageTracker::")
                   ACE_TEXT("wait_messages_pending (Redmine Issue# 1446) %p (caller: %C)\n"),
                   ACE_TEXT("Timed out waiting for messages to be transported"),
                   caller_message.c_str()));
      }
      break;
    }
  }
  if (report) {
    ACE_DEBUG((LM_DEBUG,
               "%T (%P|%t) MessageTracker::wait_messages_pending done\n"));
  }
}

int
MessageTracker::dropped_count()
{
  return dropped_count_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
