// -*- C++ -*-
//
#include "Writer.h"

#include "MessengerTypeSupportC.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/SafetyProfileStreams.h>

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>

using namespace std;

const int num_instances_per_writer = 1;
extern int num_messages;

Writer::Writer(::DDS::DataWriter_ptr writer)
  : writer_(::DDS::DataWriter::_duplicate(writer)),
    finished_instances_(0),
    timeout_writes_(0)
{
}

void Writer::start()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::start \n")));
  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate(THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Writer::start: activate failed.\n")));
    exit(1);
  }
}

void Writer::end()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::end \n")));
  wait();
}

int Writer::svc()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  ::DDS::InstanceHandleSeq handles;
  //Wait for fully association and then send.
  while (1) {
    writer_->get_matched_subscriptions(handles);
    if (handles.length() > 0) {
      break;
    } else {
      ACE_OS::sleep(1);
    }
  }

  try {
    ::Messenger::MessageDataWriter_var message_dw
      = ::Messenger::MessageDataWriter::_narrow(writer_.in());
    if (CORBA::is_nil(message_dw.in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) Writer::svc: Data Writer could not be narrowed.\n")));
      exit(1);
    }

    Messenger::Message message;
    message.subject_id = 99;
    ::DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from = CORBA::string_dup("Comic Book Guy");
    message.subject = CORBA::string_dup("Review");
    message.text = CORBA::string_dup("Worst. Movie. Ever.");
    message.count = 0;

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %T Writer::svc starting to write.\n")));
    for (int i = 0; i < num_messages; i++) {
      ::DDS::ReturnCode_t ret = message_dw->write(message, handle);

      if (ret != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) Writer::svc ERROR, ")
                   ACE_TEXT ("%dth write() returned %C.\n"),
                   i, OpenDDS::DCPS::retcode_to_string(ret)));
        if (ret == ::DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }
      message.count++;

      ACE_OS::sleep(1);
    }
  } catch (CORBA::Exception& e) {
    e._tao_print_exception("publisher: Writer: Exception caught in svc:");
    exit(1);
  }

  // wait for datareader finish.
  while (1) {
    writer_->get_matched_subscriptions(handles);
    if (handles.length() == 0) {
      break;
    } else {
      ACE_OS::sleep(1);
    }
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  finished_instances_++;

  return 0;
}

bool Writer::is_finished() const
{
  return finished_instances_ == num_instances_per_writer;
}

int Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}
