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
: writer_(::DDS::DataWriter::_duplicate(writer)),
  finished_instances_(0),
  timeout_writes_(0)
{
}

void
Writer::start()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start\n")));
  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate(THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    cerr << "Writer::start(): activate failed" << endl;
    exit(1);
  }
}

void
Writer::end()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::end\n")));
  wait();
}

namespace {
  const CORBA::ULong N = 5;

  template<class Seq>
  void seq_init(Seq& s)
  {
    s.length(N);
    for (CORBA::ULong i = 0; i < N; ++i) {
      s[i] = 0;
    }
  }

#ifdef NONNATIVE_LONGDOUBLE
  void seq_init(CORBA::LongDoubleSeq& s)
  {
    s.length(N);
    for (CORBA::ULong i = 0; i < N; ++i) {
      s[i].assign(0);
    }
  }
#endif
}


int
Writer::svc()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  try {

    while (true) {
      writer_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0, 200000));
    }

    MessageDataWriter_var message_dw
      = MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil(message_dw.in())) {
      cerr << "Data Writer could not be narrowed"<< endl;
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from       = "Comic Book Guy";
    message.subject    = "Review";
    message.text       = "Worst. Movie. Ever.";
    message.count      = 0;

    seq_init(message.bool_seq);
    seq_init(message.longdouble_seq);
    seq_init(message.short_seq);
    seq_init(message.ushort_seq);
    seq_init(message.char_seq);
    seq_init(message.longlong_seq);
    seq_init(message.wchar_seq);
    seq_init(message.double_seq);
    seq_init(message.long_seq);
    seq_init(message.ulonglong_seq);
    seq_init(message.float_seq);
    seq_init(message.octet_seq);
    seq_init(message.ulong_seq);
    seq_init(message.outside_long_seq);
    message.string_seq.length(N);
    message.wstring_seq.length(N);

    message.string_seq[4] = "test";

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));
    for (int i = 0; i < num_messages; ++i) {
      message.short_seq[4]      = message.count;
      message.long_seq[4]       = message.count;
      message.octet_seq[4]      = message.count;
      message.outside_long_seq[4] = message.count;
      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                    ACE_TEXT("%dth write() returned %d.\n"),
                    i, ret));
        if (ret == ::DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }
      message.count++;
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in svc:" << endl
         << e << endl;
  }

  while (true) {
    writer_->get_matched_subscriptions(handles);
    if (handles.length() == 0)
      break;
    else
      ACE_OS::sleep(1);
  }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  ++finished_instances_;

  return 0;
}


bool
Writer::is_finished() const
{
  return finished_instances_ == num_instances_per_writer;
}

int
Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}
