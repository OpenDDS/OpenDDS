// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "MessageTypeSupportC.h"
#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include <performance-tests/DCPS/dummyTCP/PerformanceTest.h>
#include <dds/DCPS/Qos_Helper.h>
#include <string>

using namespace Messenger;

const int num_instances_per_writer = 1;
extern int num_messages;

Writer::Writer(::DDS::DataWriter_ptr writer)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  finished_instances_ (0),
  timeout_writes_ (0)
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));
  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    cerr << "Writer::start(): activate failed" << endl;
    exit(1);
  }
}

void
Writer::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end \n")));
  wait ();
}


int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {

    while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,200000));
    }

    ::Messenger::MessageDataWriter_var message_dw
      = ::Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->_cxx_register (message);

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("%T (%P|%t) Writer::svc starting to write.\n")));
    for (int i = 0; i< num_messages; i ++) {

    // message only contain a timestamp to reduce the overhead from complex types
    // so that we can focus on the performance of transport framework.
      DDS::Time_t ddstime = OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      message.timestamp.sec = ddstime.sec;
      message.timestamp.nanosec = ddstime.nanosec;


      // begin the  performance test
      PerformanceTest::start_test ("Publisher Side Transport Performance Test",
                                   "Just before write");
      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t)ERROR  Writer::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i, ret));
        if (ret == ::DDS::RETCODE_TIMEOUT) {
          timeout_writes_ ++;
        }
      }
    }

    // the loop is done, now report stats.
    PerformanceTest::report_stats("Publisher Side Transport Performance Test");
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
	 << e << endl;
  }

  while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  finished_instances_ ++;

  return 0;
}


bool
Writer::is_finished () const
{
  return finished_instances_ == num_instances_per_writer;
}

int
Writer::get_timeout_writes () const
{
  return timeout_writes_.value ();
}
