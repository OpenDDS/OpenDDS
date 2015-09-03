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
static int const num_messages = 10;

Writer::Writer(::DDS::DataWriter_ptr writer)
: writer_ (::DDS::DataWriter::_duplicate (writer))
{
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
    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      cerr << "Data Writer could not be narrowed"<< endl;
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

    for (int i = 0; i< num_messages; i ++)
    {
      ++message.count;

      ::DDS::ReturnCode_t const ret = message_dw->write (message, handle);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i,
                    -1));
      }

      // Sleep for half a second between writes to allow some deadline
      // periods to expire.  Missed deadline should not occur since
      // the time between writes should be less than the offered
      // deadline period.
      ACE_OS::sleep (ACE_Time_Value (0, 500000));
    }
  }
  catch (CORBA::Exception& e)
  {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  ACE_DEBUG ((LM_DEBUG, "(%P|%t) Done writing. \n"));

  return 0;
}
