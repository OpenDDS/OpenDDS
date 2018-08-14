// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.h
 *
 *
 *
 */
// ============================================================================
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "DataReaderListener.h"

#include "ace/OS_NS_unistd.h"
#include "ace/SString.h"

#define MY_DOMAIN 4

#define MY_TOPIC1 "T1"
#define MY_TOPIC2 "T2"
#define MY_TOPIC3 "T3"
#define MY_TOPIC4 "T4"
#define MY_TOPIC5 "T5"
#define MY_TOPIC6 "T6"
#define MY_TOPIC7 "T7"

#define MY_TYPE1 "foo1"
#define MY_TYPE4 "foo4"

#define TOPIC_T1 1
#define TOPIC_T2 2
#define TOPIC_T3 4
#define TOPIC_T4 8
#define TOPIC_T5 16
#define TOPIC_T6 32
#define TOPIC_T7 64

static const ACE_Time_Value small_time(2, 250000);

static const int LEASE_DURATION_SEC = 5; // seconds

static ACE_TString synch_dir;
// These files need to be unlinked in the run test script before and
// after running.
static ACE_TString pub_finished_filename = ACE_TEXT("_publisher_finished.txt");
static ACE_TString sub_finished_filename = ACE_TEXT("_subscriber_finished.txt");

inline bool check_listener(const DataReaderListenerImpl* drl, int expected,
                           const ACE_TCHAR* topic)
{
  if (drl == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to dynamic cast data ")
               ACE_TEXT("reader listener\n")));
    return false;
  }

  while (drl->num_samples() < expected) {
    ACE_DEBUG((LM_INFO, "Only %d of %d samples, waiting\n",
               drl->num_samples(), expected));
    ACE_OS::sleep(small_time);
  }

  ACE_OS::printf("\n*** %s received %d samples.\n", ACE_TEXT_ALWAYS_CHAR(topic),
                 drl->num_samples());

  const ACE_TString t1_fn = topic + sub_finished_filename;
  FILE* const readers_completed = ACE_OS::fopen((synch_dir + t1_fn).c_str(), ACE_TEXT("w"));

  if (readers_completed == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Unable to create subscriber ")
               ACE_TEXT("completed file\n")));
    return false;
  }

  ACE_OS::fclose(readers_completed);
  return true;
}

inline void wait_for_file(const ACE_TCHAR* topic, const ACE_TString& suffix)
{
  FILE* file = 0;
  const ACE_TString filename = topic + suffix;

  while (!file) {
    ACE_OS::sleep(small_time);
    file = ACE_OS::fopen((synch_dir + filename).c_str(), ACE_TEXT("r"));
  }

  ACE_OS::fclose(file);
}
