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
extern bool unregister_notify_test;
extern bool dispose_notify_test;

Writer::Writer(DistributedConditionSet_rch dcs,
               ::DDS::DataWriter_ptr writer)
  : dcs_(dcs)
  , writer_ (::DDS::DataWriter::_duplicate (writer))
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start\n")));
  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    ACE_ERROR((LM_ERROR, "Writer::start(): activate failed\n"));
    exit(1);
  }
}

void
Writer::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end\n")));
  wait ();
}

int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {
    ::Messenger::MessageDataWriter_var message_dw
      = ::Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      ACE_ERROR((LM_ERROR, "Data Writer could not be narrowed\n"));
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from       = CORBA::string_dup("Comic Book Guy");
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");
    message.count      = 0;

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));
    for (int i = 0; i< num_messages; i ++) {
      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i, ret));
      }
      if (unregister_notify_test && i == num_messages/2)
      {
        ACE_DEBUG((LM_DEBUG, "unregistering instance\n"));
        message_dw->unregister_instance(message, handle);
        dcs_->wait_for("pub", "sub", "unregister");
        handle = message_dw->register_instance(message);
      }

      message.count++;
    }

    if (dispose_notify_test)
    {
      ACE_DEBUG((LM_DEBUG, "disposing instance\n"));
      message_dw->dispose (message, handle);
      dcs_->wait_for("pub", "sub", "dispose");
    }

    dcs_->wait_for("pub", "sub", "data");

  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  return 0;
}
