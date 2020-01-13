// -*- C++ -*-
//
#include "Writer.h"
#include "common.h"
#include "../common/TestException.h"
#include "../common/TestSupport.h"

#include "dds/DCPS/Service_Participant.h"

#include "tests/DCPS/ManyTopicTypes/Foo1DefTypeSupportC.h"
#include "tests/DCPS/ManyTopicTypes/Foo2DefTypeSupportC.h"
#include "tests/DCPS/ManyTopicTypes/Foo3DefTypeSupportC.h"
#include "tests/Utils/StatusMatching.h"

#include "ace/OS_NS_unistd.h"


Writer::Writer(DDS::DataWriter* writer,
               int num_thread_to_write,
               int num_writes_per_thread)
  : num_thread_to_write_(num_thread_to_write),
    num_writes_per_thread_(num_writes_per_thread),
    finished_sending_(false)
{
  ::DDS::DataWriterQos dw_qos;
  writer->get_qos(dw_qos);
  max_wait_ = dw_qos.liveliness.lease_duration.sec / 2;
}

void
Writer::start()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::start \n")));
  if (activate(THR_NEW_LWP | THR_JOINABLE, num_thread_to_write_) == -1)
  {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) Writer::start, %p.\n"),
               ACE_TEXT("activate")));
    throw TestException();
  }
}

int
Writer::wait_match(const DDS::DataWriter_var& dw, unsigned int count)
{
  return Utils::wait_match(dw, count, Utils::GTE);
}

void
Writer::end()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end \n")));
  wait();
}

bool
Writer::is_finished() const
{
  return finished_sending_;
}


void Writer::rsleep(const int wait)
{
  int lwait = 1 + (ACE_OS::rand() % wait);
  ACE_OS::sleep(ACE_Time_Value(0, lwait));
}

void Writer::rsleep1()
{
  int wait = 2 + (ACE_OS::rand() % (max_wait_ - 1)); // 2 because we want at
                                                     // least 2 seconds
  ACE_OS::sleep(wait);
}
