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

Writer::Writer(const ::DDS::DataWriter_var& writer, const OPENDDS_STRING& name, int write_count)
  : writer_(writer)
  , name_(name)
  , write_count_(write_count)
{}

void Writer::start ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start\n")));
  // Launch threads.
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    cerr << "Writer::start(): activate failed" << endl;
    exit(1);
  }
}

void Writer::end ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::end\n")));
  wait ();
}

int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;

  OPENDDS_STRING from = "Comic Book Guy";
  from += name_;

  Messenger::MessageDataWriter_var message_dw = Messenger::MessageDataWriter::_narrow(writer_.in());

  Messenger::Message message;
  message.subject_id = 99;
  ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

  message.from = from.c_str();
  message.subject = CORBA::string_dup("Review");
  message.text = CORBA::string_dup("Worst. Movie. Ever.");
  message.count = 0;

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));

  for (int i = 0; i < write_count_; i ++) {
    ++message.count;

    ::DDS::ReturnCode_t const ret = message_dw->write(message, handle);

    if (ret != ::DDS::RETCODE_OK) {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                  ACE_TEXT("%dth write() returned %d.\n"),
                  i,
                  ret));
    }
  }

  const DDS::Duration_t d = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  const ::DDS::ReturnCode_t ret = writer_->wait_for_acknowledgments(d);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: Writer::svc, ")
                ACE_TEXT("wait_for_acknowledgments returned %d.\n"),
                ret));
  }

  ACE_DEBUG ((LM_DEBUG, "(%P|%t) Done writing.\n"));

  return 0;
}
