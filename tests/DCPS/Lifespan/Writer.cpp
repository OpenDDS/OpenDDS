// -*- C++ -*-
//
#include "Writer.h"
#include "dds/DCPS/Service_Participant.h"

#include <dds/DCPS/Qos_Helper.h>

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using namespace Messenger;
using namespace std;

static int const num_instances_per_writer = 1;
static int const num_messages = 5;

Writer::Writer(::DDS::DataWriter_ptr writer)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  finished_instances_ (0),
  timeout_writes_ (0),
  start_ (true),
  count_ (0),
  dwl_servant_ (0)
{
  ::DDS::DataWriterListener_var dwl = writer->get_listener();
  dwl_servant_ =
    dynamic_cast<DataWriterListenerImpl*>(dwl.in());
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));
  // Launch threads.
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1)
  {
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
  try
  {
    Messenger::MessageDataWriter_var message_dw
      = Messenger::MessageDataWriter::_narrow (writer_.in ());
    if (CORBA::is_nil (message_dw.in ()))
    {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from       = CORBA::string_dup("Comic Book Guy");
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));

    write (message_dw.ptr(), handle, message, num_messages);
  }
  catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  ACE_DEBUG ((LM_DEBUG, "(%P|%t) Done writing. \n"));

  // After first thread sends all samples, the datareader started.

  // When the datawriter and datareader fully associated, change the start_
  // flag to let the second write thread start writing.

  // The datareader should receive all samples sent by two threads.
  while (!dwl_servant_->publication_matched())
    {
      ACE_OS::sleep(1);
    }

  this->start_ = true;

  // wait for datareader finish.
  while (1)
    {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  ++this->finished_instances_;

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

int
Writer::write (Messenger::MessageDataWriter_ptr message_dw,
               ::DDS::InstanceHandle_t& handle,
               Messenger::Message& message,
               int num_messages)
{
  for (int i = 0; i < num_messages; ++i)
  {
    ++this->count_;
    message.count = this->count_;

    // Force the data to have a timestamp in the past.
    ACE_Time_Value const timestamp (
      ACE_OS::gettimeofday () - ACE_Time_Value (1000));

    ::DDS::ReturnCode_t const ret =
        message_dw->write_w_timestamp (
          message,
          handle,
          OpenDDS::DCPS::time_value_to_time (timestamp));

    if (ret != ::DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                  ACE_TEXT ("%dth write() returned %d.\n"),
                  i,
                  -1));

      if (ret == ::DDS::RETCODE_TIMEOUT)
      {
        ++this->timeout_writes_;
      }
    }

    // Sleep half a second in between writes.
    ACE_OS::sleep (ACE_Time_Value (0, 500000));
  }

  // Write a sample with the current timestamp.  It should be received
  // by the subscriber.
  ++this->count_;
  message.count = this->count_;

  ::DDS::ReturnCode_t const ret = message_dw->write (message, handle);

  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                ACE_TEXT ("write() returned %d.\n"),
                -1));

    if (ret == ::DDS::RETCODE_TIMEOUT)
      {
        ++this->timeout_writes_;
      }
  }

  return 0;
}
