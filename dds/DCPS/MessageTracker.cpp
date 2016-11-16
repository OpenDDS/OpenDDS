/*
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ace/Synch.h"
#include <dds/DCPS/MessageTracker.h>
#include <dds/DCPS/Service_Participant.h>
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
, done_condition_(lock_)
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
    ACE_TCHAR date_time[50];
    ACE_TCHAR* const time =
      MessageTracker::timestamp(pending_timeout,
                                date_time,
                                50);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%T (%P|%t) MessageTracker::wait_messages_pending ")
               ACE_TEXT("from source=%C will wait until %s.\n"),
               msg_src_.c_str(),
               (pTimeout == 0 ? ACE_TEXT("(no timeout)") : time)));
  }
  while (true) {
    if (!pending_messages())
      break;

    if (done_condition_.wait(pTimeout) == -1 && pending_messages()) {
      if (DCPS_debug_level) {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("(%P|%t) %T MessageTracker::")
                   ACE_TEXT("wait_messages_pending (Redmine Issue# 1446) %p (caller: %s)\n"),
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

ACE_TCHAR *
MessageTracker::timestamp (const ACE_Time_Value& time_value,
                           ACE_TCHAR date_and_time[],
                           size_t date_and_timelen)
{
  //ACE_TRACE ("ACE::timestamp");

  // This magic number is from the formatting statement
  // farther down this routine.
  if (date_and_timelen < 27)
    {
      errno = EINVAL;
      return 0;
    }

  ACE_Time_Value cur_time =
    (time_value == ACE_Time_Value::zero) ?
        ACE_Time_Value (ACE_OS::gettimeofday ()) : time_value;
  time_t secs = cur_time.sec ();
  struct tm tms;
  ACE_OS::localtime_r (&secs, &tms);
  ACE_OS::snprintf (date_and_time,
                    date_and_timelen,
                    ACE_TEXT ("%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%06ld"),
                    tms.tm_year + 1900,
                    tms.tm_mon + 1,
                    tms.tm_mday,
                    tms.tm_hour,
                    tms.tm_min,
                    tms.tm_sec,
                    static_cast<long> (cur_time.usec()));
  date_and_time[date_and_timelen - 1] = '\0';
  return &date_and_time[11];
}


int
MessageTracker::dropped_count()
{
  return dropped_count_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
