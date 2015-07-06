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

static int const num_messages = 5;

Writer::Writer(::DDS::DataWriter_ptr writer,
               CORBA::Long key,
               ACE_Time_Value sleep_duration)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  register_condition_ (this->register_lock_),
  sending_condition_ (this->sending_lock_),
  registered_ (false),
  instance_handle_ (::DDS::HANDLE_NIL),
  key_ (key),
  sleep_duration_ (sleep_duration),
  failed_registration_ (false),
  start_sending_ (false),
  ready_to_send_ (false)
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));
  // Launch threads.
  if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1)
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

  try
  {
    while (1)
    {
      ::DDS::InstanceHandleSeq handles;
      this->writer_->get_matched_subscriptions (handles);
      if (handles.length () == 2)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,250000));
    }

    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = this->key_;
    this->instance_handle_ = message_dw->register_instance(message);

    {
      GuardType guard (this->register_lock_);
      this->registered_ = true;
      this->register_condition_.signal ();
    }

    message.from       = CORBA::string_dup("Comic Book Guy");
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");
    message.count      = 0;

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc sleep for %d.%d seconds.\n"),
              this->sleep_duration_.sec(),this->sleep_duration_.usec()));

    {
      GuardType guard (this->sending_lock_);
      ready_to_send_ = true;
      if (! this->start_sending_)
        this->sending_condition_.wait ();
    }

    if (this->instance_handle_ == ::DDS::HANDLE_NIL)
    {
      failed_registration_ = true;
      ACE_DEBUG ((LM_INFO,
                  ACE_TEXT("(%P|%t) INFO: Writer::svc, ")
                  ACE_TEXT ("instance registration failed.\n")));
      return 0;
    }

    ACE_OS::sleep (this->sleep_duration_);

    for (int i = 0; i< num_messages; i ++)
    {
      ++message.count;

      ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc write sample %d to instance %d.\n"),
              message.count, this->instance_handle_));

      ::DDS::ReturnCode_t const ret = message_dw->write (message, this->instance_handle_);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i,
                    ret));
      }

      ACE_OS::sleep (sleep_duration_);
    }
  }
  catch (CORBA::Exception& e)
  {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  return 0;
}


::DDS::InstanceHandle_t
Writer::get_instance_handle()
{
  return this->instance_handle_;
}


bool Writer::wait_for_registered ()
{
  GuardType guard (this->register_lock_);

  if (! registered_)
  {
    ACE_Time_Value abs = ACE_OS::gettimeofday () + ACE_Time_Value (10);
    if (this->register_condition_.wait (&abs) == -1)
    {
      return false;
    }
  }
  return true;
}

bool Writer::failed_registration() const
{
  return failed_registration_;
}


void Writer::start_sending ()
{
  GuardType guard (this->sending_lock_);

  while (!ready_to_send_)
  {
    ACE_Time_Value abs = ACE_OS::gettimeofday () + ACE_Time_Value (0,250000);
    this->sending_condition_.wait (&abs);
  }

  this->start_sending_ = true;
  this->sending_condition_.signal ();
}
