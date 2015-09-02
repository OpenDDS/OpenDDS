// -*- C++ -*-
//
#include "Writer.h"
#include "MessengerTypeSupportC.h"
#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include <ace/OS_NS_wchar.h>

using namespace Messenger;
using namespace std;

const int num_instances_per_writer = 1;
const int num_messages = 10;

CORBA::Char* charseq = (CORBA::Char*)("I'm char seq");
CORBA::WChar* wcharseq = (CORBA::WChar*)(L"I'm wchar seq");
const CORBA::Char* strseq = "I'm string seq";
const CORBA::WChar* wstrseq = (const CORBA::WChar*)(L"I'm wstring seq");
const CORBA::Char* str = "I'm string";
const CORBA::WChar* wstr = (const CORBA::WChar*)(L"I'm wstring");

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

    MessageDataWriter_var message_dw
      = MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil (message_dw.in ())) {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance (message);

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));
    for (int i = 0; i< num_messages; i ++) {
      set_message (message, i);
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
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl << e << endl;
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


void Writer::set_message (Messenger::Message& message, const int& count)
{
  message.count = count;
  switch (count)
  {
    case 9:
      {
        CORBA::ULong len = static_cast<CORBA::ULong>(ACE_OS::strlen(wcharseq));
        message.bounded_wchar_seq = BoundedWCharSeq (len + 1, wcharseq);
      }
    case 8:
      {
        CORBA::ULong len = static_cast<CORBA::ULong>(ACE_OS::strlen(charseq));
        message.bounded_char_seq = BoundedCharSeq (len + 1, charseq);
      }
    case 7:
      {
        message.wstring_seq.length (5);
        message.wstring_seq[4] = CORBA::wstring_dup (wstrseq);
      }
    case 6:
      {
        message.string_seq.length (5);
        message.string_seq[4] = CORBA::string_dup (strseq);
      }
    case 5:
      {
        CORBA::ULong len = static_cast<CORBA::ULong>(ACE_OS::strlen(wcharseq));
        message.wchar_seq.replace (len + 1, len + 1, wcharseq);
      }
    case 4:
      {
        CORBA::ULong len = static_cast<CORBA::ULong>(ACE_OS::strlen(charseq));
        message.char_seq.replace (len + 1, len + 1, charseq);
      }
    case 3:
      {
        message.wstr = CORBA::wstring_dup (wstr);
      }
    case 2:
      {
        message.str = CORBA::string_dup (str);
      }
    case 1:
      {
        message.wch = L'B';
      }
    case 0:
      {
        message.ch = 'A';
      }
    break;
    default:
      ACE_ERROR ((LM_ERROR,
                  "(%P|%t) ERROR: unknown message\n"));
    break;
  }
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
