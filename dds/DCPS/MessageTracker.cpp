/*
 * $Id$
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include <dds/DCPS/MessageTracker.h>
#include <dds/DCPS/Service_Participant.h>
#include "ace/Guard_T.h"

using namespace OpenDDS::DCPS;

MessageTracker::MessageTracker()
: dropped_count_(0)
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
MessageTracker::wait_messages_pending()
{
  ACE_Time_Value pending_timeout =
    TheServiceParticipant->pending_timeout();

  ACE_Time_Value* pTimeout = 0;

  if (pending_timeout != ACE_Time_Value::zero) {
    pTimeout = &pending_timeout;
    pending_timeout += ACE_OS::gettimeofday();
  }

  ACE_GUARD(ACE_Thread_Mutex, guard, this->lock_);
  const bool report = DCPS_debug_level > 0 && pending_messages();
  if (report) {
    ACE_DEBUG((LM_DEBUG,
               "%T MessageTracker::wait_messages_pending %C\n",
               (pending_timeout == ACE_Time_Value::zero ?
                  " (no timeout)" : "")));
  }
  while (true) {
    if (!pending_messages())
      break;

    if (done_condition_.wait(pTimeout) == -1 && !pending_messages()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) MessageTracker::wait_messages_pending %p\n")
        ACE_TEXT("Timed out waiting for messages to be transported")));
      break;
    }
  }
  if (report) {
    ACE_DEBUG((LM_DEBUG,
               "%T MessageTracker::wait_messages_pending done\n"));
  }
}

int
MessageTracker::dropped_count()
{
  return dropped_count_;
}
