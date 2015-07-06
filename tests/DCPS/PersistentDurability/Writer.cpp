
#include "Writer.h"
#include <ace/OS_NS_unistd.h>
#include <iostream>

using namespace Messenger;
using namespace std;

int const num_instances_per_writer = 2;
int const num_messages = 5;

Writer::Writer (::DDS::DataWriter_ptr writer)
  : writer_ (::DDS::DataWriter::_duplicate (writer))
  , timeout_writes_ (0)
  , count_ (0)
  , dwl_servant_ (0)
{
  ::DDS::DataWriterListener_var dwl = writer->get_listener ();
  this->dwl_servant_ =
    dynamic_cast<DataWriterListenerImpl*> (dwl.in ());
}

Writer::~Writer ()
{
}


int
Writer::svc ()
{
  if (this->dwl_servant_ == 0)
    return -1;  // Should not occur.

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try
  {
    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow (writer_.in ());

    if (CORBA::is_nil (message_dw.in ()))
    {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from    = CORBA::string_dup ("Comic Book Guy");
    message.subject = CORBA::string_dup ("Review");
    message.text    = CORBA::string_dup ("Worst. Movie. Ever.");

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT ("(%P|%t) %T Writer::svc starting to write.\n")));

    this->write (message_dw.in (), handle, message);

  }
  catch (CORBA::Exception& e)
  {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  return 0;
}

bool
Writer::start ()
{
  ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) Starting Writer \n")));

  // Launch threads.
  if (this->activate (THR_NEW_LWP | THR_JOINABLE,
                      num_instances_per_writer) == -1)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("(%P|%t) %p\n"),
                       ACE_TEXT ("Error activating threads.\n")),
                      false);
  }

  return true;
}

bool
Writer::end ()
{
  int const result = this->wait ();

  if (result != 0)
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT ("(%P|%t) %p\n"),
                ACE_TEXT ("Error waiting for threads.\n")));
  else
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("(%P|%t) Done writing. \n")));

  return result == 0;
}

int
Writer::get_timeout_writes () const
{
  return timeout_writes_.value ();
}

int
Writer::write (Messenger::MessageDataWriter_ptr message_dw,
               ::DDS::InstanceHandle_t& handle,
               Messenger::Message& message)
{
  for (int i = 0; i < num_messages; ++i)
  {
    ++this->count_;
    message.count = this->count_;

    ::DDS::ReturnCode_t const ret = message_dw->write (message, handle);

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

  }

  return 0;
}
