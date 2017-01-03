// -*- C++ -*-
//
#include "Writer.h"
#include "MessengerTypeSupportC.h"
#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using namespace Messenger;
using namespace std;

const int num_instances_per_writer = 1;
const int num_messages = 10;

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
    message.subject_id = "OpenDDS";
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from       = "Comic Book Guy";
    message.subject    = "Review";
    message.text       = "Worst. Movie. Ever.";
    message.count      = 0;

    message.e[0] = d;
    message.e[1] = s;
    message.e[2] = b;

    message.seq.length(2);
    message.seq[0] = other2;
    message.seq[1] = other1;

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));
    for (int i = 0; i< num_messages; i ++) {
      switch (i % 4) {
      case 0:
        message.u.u_b (true);
        break;
      case 1:
        message.u.u_s (534);
        break;
      case 2:
        message.u.u_d (1.345);
        break;
      case 3:
        message.u.u_f (3.14f);
        message.u._d (other2);
        break;
      }

      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i, ret));
        if (ret == ::DDS::RETCODE_TIMEOUT) {
          timeout_writes_ ++;
        }
      }

      message.count++;
    }


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
